#include "connection.h"
#include "io_utils.h"
#include "format.h"
#include "libs/dictionary.h"
#include "libs/callbacks.h"
#include "server.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <err.h>

#define MAX_FILE_DESCRIPTORS 1024
#define TIMEOUT_MS 1000

/// @todo add a changelist so we don't have to call kevent a bunch of times - maybe a new function? + function to reset changelist
#if defined(__APPLE__)
#include <sys/event.h>
static struct kevent* events_array = NULL;
static struct kevent* change_list = NULL;
static size_t changes_queued = 0;
#elif defined(__linux__)
#include <sys/epoll.h>
static struct epoll_event* events_array = NULL;
#endif

static int event_queue_fd = 0;
static int stop_server = 0;
static int server_socket = 0;
static int was_server_initialized = 0;

// Configure the server socket.
void server_setup_socket(char* port) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int ret = getaddrinfo(NULL, port, &hints, &result);
    if ( ret ) {
        freeaddrinfo(result);
        errx(EXIT_FAILURE, "getaddrinfo: %s", gai_strerror(ret));
    }

    if ( bind(server_socket, result->ai_addr, result->ai_addrlen) ) {
        freeaddrinfo(result);
        err(EXIT_FAILURE, "bind");
    }

    freeaddrinfo(result);

    if ( listen(server_socket, SOMAXCONN) ) 
        err(EXIT_FAILURE, "listen");

    make_socket_non_blocking(server_socket);
}

// Gracefully capture SIGINT and stop the server.
void handle_sigint(int signal) { (void)signal; stop_server = 1; }

// Gracefully capture SIGPIPE and do nothing.
void handle_sigpipe(int signal) { (void)signal; WARN("SIGPIPE"); }

// Configure the server's resources.
void server_setup_resources(void) {
#if defined(__APPLE__)
    events_array = calloc(MAX_FILE_DESCRIPTORS, sizeof(struct kevent));
    change_list = calloc(MAX_FILE_DESCRIPTORS, sizeof(struct kevent));
#elif defined(__linux__)
    events_array = calloc(MAX_FILE_DESCRIPTORS, sizeof(struct epoll_event));
#endif
    struct sigaction sigint_action, sigpipe_action;
    memset(&sigint_action, 0, sizeof(sigint_action));
    memset(&sigpipe_action, 0, sizeof(sigpipe_action));

    sigint_action.sa_handler = handle_sigint;
    sigpipe_action.sa_handler = handle_sigpipe;

    if ( sigaction(SIGINT, &sigint_action, NULL) < 0 )
        err(EXIT_FAILURE, "sigaction SIGINT");

    if ( sigaction(SIGPIPE, &sigpipe_action, NULL) < 0 )
        err(EXIT_FAILURE, "sigaction SIGPIPE");
}

void queue_event_change(int16_t filter, int fd, void* data) {
    LOG("Adding filter %d on fd=%d", filter, fd);
    EV_SET(change_list + changes_queued, fd, filter, EV_ADD, 0, 0, data);  // add EV_CLEAR?
    ++changes_queued;
}

void reset_queued_events(void) { 
    memset(change_list, 0, changes_queued * sizeof(kevent));
    changes_queued = 0; 
}

// Handle an kqueue event triggered from a client requesting to connect.
int server_handle_new_client(void) {
    struct sockaddr_in client_addr = { 0 };
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(server_socket, (struct sockaddr*) &client_addr, &len);

    if (client_fd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG("accept returned EAGAIN or EWOULDBLOCK");
        } else {
            err(EXIT_FAILURE, "accept");
        }

        return -1;
    } else {
        if (client_fd >= MAX_FILE_DESCRIPTORS) 
            errx(EXIT_FAILURE, "client_fd (%d) >= MAX_FILE_DESCRIPTORS (%d)", 
                client_fd, MAX_FILE_DESCRIPTORS);

        make_socket_non_blocking(client_fd);
        connection_initializer_t c_init = { 0 };
        c_init.client_fd = client_fd;
        c_init.client_port = htons(client_addr.sin_port);
        c_init.client_address = inet_ntoa(client_addr.sin_addr);
        connection_t* connection = connection_init(&c_init);
    
#if defined(__APPLE__)
        queue_event_change(EVFILT_READ, client_fd, connection);
        // struct kevent client_event;
        // uint16_t flags = EV_ADD; // add EV_CLEAR?
        // EV_SET(&client_event, client_fd, EVFILT_READ, flags, 0, 0, connection);

        // if ( kevent(event_queue_fd, &client_event, 1, NULL, 0, NULL) == -1 ) 
        //     err(EXIT_FAILURE, "kevent register");
        
        // if ( client_event.flags & EV_ERROR )
        //     errx(EXIT_FAILURE, "Event error: %s", strerror(client_event.data));
#elif defined(__linux__)
        struct epoll_event event = {0};
        LOG("creating epoll event for fd=%d", client_fd);
        event.data.ptr = connection;
        event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLHUP;

        if ( epoll_ctl(event_queue_fd, EPOLL_CTL_ADD, client_fd, &event) < 0 )
            err(EXIT_FAILURE, "epoll_ctl EPOLL_CTL_ADD");
#endif
        print_client_connected(c_init.client_address, c_init.client_port, client_fd);

        return client_fd;
    }
}

// Handle an kqueue event from a client connection.
void server_handle_client(connection_t* c, size_t event_data) {
    /// @todo split function into 2 for handling read and handling write
    if ( c->state < CS_REQUEST_RECEIVED ) {
        if ( connection_read(c, event_data) <= 0 ) { return; }
        LOG("%s", c->buf);
    }

    if ( c->state == CS_CLIENT_CONNECTED )
        connection_try_parse_verb(c);
    
    if ( c->state == CS_METHOD_PARSED )
        connection_try_parse_url(c);

    if ( c->state == CS_URL_PARSED )
        connection_try_parse_protocol(c);

    if ( c->state == CS_REQUEST_PARSED ) {
        connection_try_parse_headers(c);
        connection_shift_buffer(c);
    }

    if ( c->state == CS_HEADERS_PARSED )
        connection_read_request_body(c);

    if ( c->state == CS_REQUEST_RECEIVED ) {
        c->response = handle_route(c->request);
        c->state = CS_WRITING_RESPONSE_HEADER;

#if defined(__APPLE__)
        /// @todo only add this event if we need to continue writing to the fd after this function finishes
        queue_event_change(EVFILT_WRITE, c->client_fd, c);
        // struct kevent change_rd_to_wr, new_wr_event;
        // uint16_t flags = EV_ADD; // add EV_CLEAR?
        // EV_SET(&change_rd_to_wr, c->client_fd, EVFILT_WRITE, flags, 0, 0, c);

        // if ( kevent(event_queue_fd, &change_rd_to_wr, 1, &new_wr_event, 1, 0) == -1 ) 
        //     err(EXIT_FAILURE, "kevent register");

        event_data = free_bytes_in_wr_socket(c->client_fd);
#endif
    }

    if ( c->state == CS_WRITING_RESPONSE_HEADER )
        connection_write_response_header(c);

    if ( c->state == CS_WRITING_RESPONSE_BODY ) {
        if ( !connection_try_send_response_body(c, event_data) ) {
            const char* http_method_str = http_method_to_string(c->request->method);
            const char* http_status_str = http_status_to_string(c->response->status);

            print_client_request_resolution(
                c->client_address, c->client_port, http_method_str, 
                c->request->path, c->request->protocol, c->response->status, 
                http_status_str, c->body_bytes_to_receive, 
                c->body_bytes_to_transmit, &c->time_connected,
                &c->time_received, &c->time_begin_send
            );

#if defined(__APPLE__)
            // queue_event_change(EV_DELETE, c->client_fd, c);
#elif defined(__linux__)
            if (epoll_ctl(event_queue_fd, EPOLL_CTL_DEL, c->client_fd, NULL) < 0)
                err(EXIT_FAILURE, "epoll_ctl EPOLL_CTL_DEL");
#endif
            connection_destroy(c);
        }
    }
}

// Cleanup the resources used by the server. Called on program exit.
void server_cleanup(void) {
    LOG("Exiting server...");
    if ( events_array ) 
        free(events_array);

    if ( change_list ) 
        free(change_list);
    
    close(server_socket);
}

void server_init(char* port) {
    if ( was_server_initialized )
        errx(EXIT_FAILURE, "Cannot initialize server twice");

    if ( !port ) 
        errx(EXIT_FAILURE, "Cannot bind to NULL port");

    was_server_initialized = 1;
    atexit(server_cleanup);
    server_setup_socket(port);
    print_server_details(port);
    server_setup_resources();

#if defined(__APPLE__)
    event_queue_fd = kqueue();
    if (event_queue_fd == -1)
        err(EXIT_FAILURE, "kqueue");
    
    struct kevent accept_event; 
    EV_SET(&accept_event, server_socket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);

    if (kevent(event_queue_fd, &accept_event, 1, NULL, 0, NULL) == -1) 
        err(EXIT_FAILURE, "kevent register");

    if (accept_event.flags & EV_ERROR)
        errx(EXIT_FAILURE, "Event error: %s", strerror(accept_event.data));
#elif defined(__linux__)
    event_queue_fd = epoll_create1(0);
    if (event_queue_fd == -1) 
        err(EXIT_FAILURE, "epoll_create1");

    struct epoll_event accept_event = { 0 };
    accept_event.data.fd = server_socket;
    accept_event.events = EPOLLIN;
    if (epoll_ctl(event_queue_fd, EPOLL_CTL_ADD, server_socket, &accept_event) < 0) 
        err(EXIT_FAILURE, "epoll_ctl EPOLL_CTL_ADD");
#endif
}

void server_launch(void) {
    print_server_ready();

    int num_events = 0;
    while ( !stop_server ) {
#if defined(__APPLE__)
        LOG("-------------------------------------")
        num_events = kevent(event_queue_fd, change_list, changes_queued, events_array, MAX_FILE_DESCRIPTORS, 0);
        reset_queued_events();
#elif defined(__linux__)
        num_events = epoll_wait(event_queue_fd, events_array, MAX_FILE_DESCRIPTORS, TIMEOUT_MS);
#endif
        if ( num_events == -1 )  { break; }

        for(int i = 0 ; i < num_events; i++) {
#if defined(__APPLE__)
            int fd = events_array[i].ident;
#elif defined(__linux__)
            int fd = events_array[i].data.fd;
#endif
            if (fd == server_socket) { 
                // we have a new connection to the server
                server_handle_new_client();
            } else {
#if defined(__APPLE__)
                connection_t* connection = events_array[i].udata;
                size_t event_data = events_array[i].data;

                if ( events_array[i].flags & EV_EOF ) {
                    WARN("client on fd=%d disconnected", connection->client_fd);
                    close(fd);
                    continue;
                } 
                else if ( events_array[i].flags & EV_ERROR ) {
                    // errx(EXIT_FAILURE, "Event error: %s on fd=%d", strerror(events_array[i].data), fd);
                    WARN("Event error: %s on fd=%d", strerror(events_array[i].data), fd);
                    continue;
                }
#elif defined(__linux__)
                connection_t* connection = events_array[i].data.ptr;
                size_t event_data = num_bytes_in_rd_socket(connection->client_fd);

                if ( events_array[i].events & (EPOLLRDHUP | EPOLLHUP) ) {
                    WARN("client on fd=%d disconnected", connection->client_fd);
                    close(fd);
                    if (epoll_ctl(event_queue_fd, EPOLL_CTL_DEL, connection->client_fd, NULL) < 0)
                        err(EXIT_FAILURE, "epoll_ctl EPOLL_CTL_DEL");
                    continue;
                }
#endif
                server_handle_client(connection, event_data);
            }
        }
    }
}

void server_register_route(http_method method, char* route, request_handler_t handler) {
    register_route(method, route, handler);
}