#include "internals/connection.h"
#include "internals/io_utils.h"
#include "internals/format.h"
#include "libs/dictionary.h"
#include "libs/callbacks.h"
#include "protocol.h"
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

#if defined(__APPLE__)
#include <sys/event.h>
static struct kevent* events_array = NULL;
#elif defined(__linux__)
#include <sys/epoll.h>
static struct epoll_event* events_array = NULL;
#endif

static int event_queue_fd = 0;
static int stop_server = 0;
static int server_socket = 0;
static int was_server_initialized = 0;

static dictionary* routes = NULL;
static dictionary* connections = NULL;

// Format: (header_name, header_value)
static const char* HEADER_FMT = "%s: %s\r\n";

// Format: (response_code, response_string, all_headers, response_body)
// Assumes that headers come with \r\n at the end
static const char* RESPONSE_HEADER_FMT = "HTTP/1.0 %d %s\r\n%s\r\n";

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
#if defined(__APPLE__)
    events_array = calloc(MAX_FILE_DESCRIPTORS, sizeof(struct kevent));
#elif defined(__linux__)
    events_array = calloc(MAX_FILE_DESCRIPTORS, sizeof(struct epoll_event));
#endif
    
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
    
#if defined(__APPLE__)
        struct kevent client_event;
        EV_SET(&client_event, client_fd, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, NULL);

        if ( kevent(event_queue_fd, &client_event, 1, NULL, 0, NULL) == -1 ) 
            err(EXIT_FAILURE, "kevent register");
        
        if ( client_event.flags & EV_ERROR )
            errx(EXIT_FAILURE, "Event error: %s", strerror(client_event.data));
#elif defined(__linux__)
        struct epoll_event event = {0};
        event.data.fd = client_fd;
        event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLHUP;

        if ( epoll_ctl(event_queue_fd, EPOLL_CTL_ADD, client_fd, &event) < 0 )
            perror("epoll_ctl EPOLL_CTL_ADD");
#endif
        char* client_addr_str = inet_ntoa(client_addr.sin_addr);
        uint16_t client_port = htons(client_addr.sin_port);
        print_client_connected(client_addr_str, client_port);

        return client_fd;
    }
}

int format_response_header(response_t* response, char** buffer) {
    vector* defined_header_keys = dictionary_keys(response->headers);
    vector* formatted_headers = string_vector_create();
    int allocated_space = 1; // allow for NUL-byte at the end

    for (size_t i = 0; i < vector_size(defined_header_keys); ++i) {
        char* key = vector_get(defined_header_keys, i);
        char* value = dictionary_get(response->headers, key);
        char* buffer = NULL;
        allocated_space += asprintf(&buffer, HEADER_FMT, key, value);

        vector_push_back(formatted_headers, buffer);
        free(buffer);
    }

    char* joined_headers = calloc(1, allocated_space);
    for (size_t i = 0; i < vector_size(formatted_headers); ++i)
        strcat(joined_headers, vector_get(formatted_headers, i));
    
    strcat(joined_headers, "\0");

    const char* status_str = http_status_to_string(response->status);
    int ret = asprintf(buffer, RESPONSE_HEADER_FMT, response->status, status_str, joined_headers);
    
    vector_destroy(defined_header_keys);
    vector_destroy(formatted_headers);
    free(joined_headers);

    return ret;
}

response_t* handle_response_marshalling(request_t* request) {
    char* requested_route = request->path;
    response_t* response = NULL;

    if ( request->method == HTTP_UNKNOWN ) {
        response = response_bad_request();
    } else if ( !dictionary_contains(routes, requested_route) ) {
        response = response_resource_not_found();
    } else {
        request_handler_t* handlers = dictionary_get(routes, requested_route);
        request_handler_t handler = handlers[request->method];

        if ( !handler ) {
            response = response_method_not_allowed();
        } else {
            response = handler(request);
        }
    }

    return response;
}

// Handle an kqueue event from a client connection.
void server_handle_client(int client_fd) {
    connection_t* conn = dictionary_get(connections, &client_fd);
    if ( connection_read(conn) <= 0 ) { return; }

    if ( conn->state == CS_CLIENT_CONNECTED )
        connection_try_parse_verb(conn);
    
    if ( conn->state == CS_METHOD_PARSED )
        connection_try_parse_url(conn);

    if ( conn->state == CS_URL_PARSED ) {
        connection_try_parse_protocol(conn);

        LOG("[%s] [%s] [%s]", http_method_to_string(conn->request->method), conn->request->path, conn->request->protocol);
    }

    if ( conn->state == CS_REQUEST_PARSED ) /// @todo parse request body here
        connection_try_parse_headers(conn);

    if ( conn->state == CS_HEADERS_PARSED ) { 
        conn->state = CS_REQUEST_RECEIVED;
    }

    if ( conn->state == CS_REQUEST_RECEIVED ) {
        conn->response = handle_response_marshalling(conn->request);
        conn->state = CS_WRITING_RESPONSE;
    }

    if ( conn->state == CS_WRITING_RESPONSE ) {
        /// @todo format and send the conn->response field here
        /// maybe create another enum and split the sending of the response header and response content?
         
        char* header_str = NULL;
        int header_len = format_response_header(conn->response, &header_str);
        LOG("[%s]", header_str);
        write_all_to_socket(client_fd, header_str, header_len);

        size_t body_len = 0;
        sscanf(dictionary_get(conn->response->headers, "Content-Length"), "%zu", &body_len);

        if ( conn->response->rt == RT_FILE ) {
            buffered_write_to_socket(conn->client_fd, conn->response->body_content.file, body_len);
        } else if (conn->response->rt == RT_STRING) {
            write_all_to_socket(client_fd, conn->response->body_content.body, body_len);
        }

        free(header_str);

#if defined(__linux__)
        if (epoll_ctl(event_queue_fd, EPOLL_CTL_DEL, conn->client_fd, NULL) < 0)
            perror("epoll_ctl EPOLL_CTL_DEL");
#endif

        dictionary_remove(connections, &client_fd);
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
        num_events = kevent(event_queue_fd, 0, 0, events_array, MAX_FILE_DESCRIPTORS, 0);
#elif defined(__linux__)
        num_events = epoll_wait(event_queue_fd, events_array, MAX_FILE_DESCRIPTORS, TIMEOUT_MS);
#endif
        if ( num_events == -1 )  { break; }

        for(int i = 0 ; i < num_events; i++) {
#if defined(__APPLE__)
            int fd = events_array[i].ident;
            // events_array[i].data;
#elif defined(__linux__)
            int fd = events_array[i].data.fd;
            // events_array[i].events;
#endif     
            if (fd == server_socket) { 
                // we have a new connection to the server
                server_handle_new_client();
            } else if (fd > 0) {
                server_handle_client(fd);
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