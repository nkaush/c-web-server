#include "internals/connection.h"
#include "internals/common.h"
#include "internals/format.h"
#include "libs/dictionary.h"
#include "libs/callbacks.h"
#include "server.h"

#include <sys/socket.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <err.h>

#define MAX_FILE_DESCRIPTORS 1024

static int kq_fd = 0;
static int stop_server = 0;
static int server_socket = 0;
static int was_server_initialized = 0;

static dictionary* routes = NULL;
static dictionary* connections = NULL;

static struct kevent* events_array = NULL;

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
void handle_sigpipe(int signal) { (void)signal; }

void* allocate_handler_array(void* ptr) {
    (void)ptr;
    return calloc(NUM_HTTP_METHODS, sizeof(request_handler_t));
}

// Configure the server's resources.
void server_setup_resources(void) {
    events_array = calloc(MAX_FILE_DESCRIPTORS, sizeof(struct kevent));
    connections = dictionary_create(
        int_hash_function, int_compare, 
        int_copy_constructor, int_destructor,
        connection_init, connection_destroy
    );

    routes = dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor, 
        allocate_handler_array, free
    );

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
            errx(EXIT_FAILURE, "client_fd (%d) >= MAX_FILE_DESCRIPTORS (%d)", client_fd, MAX_FILE_DESCRIPTORS);

        make_socket_non_blocking(client_fd);
        dictionary_set(connections, &client_fd, &client_fd);
    
        struct kevent client_event;
        EV_SET(&client_event, client_fd, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, NULL);

        if ( kevent(kq_fd, &client_event, 1, NULL, 0, NULL) == -1 ) 
            err(EXIT_FAILURE, "kevent register");
        
        if (client_event.flags & EV_ERROR)
            errx(EXIT_FAILURE, "Event error: %s", strerror(client_event.data));

        char* client_addr_str = inet_ntoa(client_addr.sin_addr);
        uint16_t client_port = htons(client_addr.sin_port);
        print_client_connected(client_addr_str, client_port);

        return client_fd;
    }
}

// Handle an kqueue event from a client connection.
void server_handle_client(int client_fd, struct kevent* event) {
    connection_t* conn = dictionary_get(connections, &client_fd);
    connection_read(conn);

    if ( conn->state == CS_CLIENT_CONNECTED ) {
        connection_try_parse_verb(conn);
    }
    
    if ( conn-> state == CS_VERB_PARSED ) {
        LOG("%d", conn->request_method);
        LOG("%s", conn->buf);
        LOG("[%s]", conn->buf + conn->buf_ptr);
    }

    if ( conn-> state == CS_REQUEST_PARSED ) {

    }

    if ( conn-> state == CS_HEADERS_PARSED ) {

    }

    if ( conn-> state == CS_REQUEST_RECEIVED ) {

    }

    if ( conn-> state == CS_WRITING_RESPONSE ) {

    }
}

// Cleanup the resources used by the server. Called on program exit.
void server_cleanup(void) {
    LOG("Exiting server...");
    if ( connections )
        dictionary_destroy(connections);

    if ( routes )
        dictionary_destroy(routes);

    if ( events_array ) 
        free(events_array);
    
    close(server_socket);
}

void server_init(char* port) {
    if ( was_server_initialized )
        errx(EXIT_FAILURE, "Cannot initialize server twice");

    else if ( !port ) 
        errx(EXIT_FAILURE, "Cannot bind to NULL port");

    was_server_initialized = 1;
    atexit(server_cleanup);
    server_setup_socket(port);
    print_server_details(port);
    server_setup_resources();

    kq_fd = kqueue();
    if (kq_fd == -1)
        err(EXIT_FAILURE, "kqueue");

    struct kevent accept_event; 
    EV_SET(&accept_event, server_socket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);

    if (kevent(kq_fd, &accept_event, 1, NULL, 0, NULL) == -1) 
        err(EXIT_FAILURE, "kevent register");
    if (accept_event.flags & EV_ERROR)
        errx(EXIT_FAILURE, "Event error: %s", strerror(accept_event.data));
}

void server_launch(void) {
    print_server_ready();

    int num_events = 0;
    while ( !stop_server ) {
        num_events = kevent(kq_fd, 0, 0, events_array, MAX_FILE_DESCRIPTORS, 0);
        if ( num_events == -1 )  { break; }

        for(int i = 0 ; i < num_events; i++) {
            int fd = events_array[i].ident;
            // events_array[i].data;
            if (fd == server_socket) { 
                // we have a new connection to the server
                server_handle_new_client();
            } else if (fd > 0) {
                server_handle_client(fd, events_array + i);

                
                // connection_destroy(dictionary_get(connections, &fd));
                // server_handle_client(events_array[i].ident, events_array[i].flags);
                // dictionary_remove(connections, &this->client_fd);
            }
        }
    }
}

void server_register_route(http_method method, char* route, request_handler_t handler) {
    if ( method == HTTP_UNKNOWN )
        errx(EXIT_FAILURE, "Cannot register handler for HTTP_UNKNOWN");
    else if ( !route ) 
        errx(EXIT_FAILURE, "Cannot register handler for NULL route");
    else if ( !handler )
        errx(EXIT_FAILURE, "Cannot register handler for NULL handler");

    if ( !dictionary_contains(routes, route) )
        dictionary_set(routes, route, NULL);

    request_handler_t* method_array = dictionary_get(routes, route);
    if ( method_array[(size_t) method] )
        WARN("Redefinition of route '%s %s'", http_method_to_string(method), route);

    method_array[(size_t) method] = handler;
}