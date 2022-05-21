#include "utils/dictionary.h"
#include "utils/set.h"
#include "common.h"
#include "format.h"
#include "server.h"

#include <sys/socket.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <err.h>

#define MAX_FILE_DESCRIPTORS 1024
#define TIMEOUT_MS 1000

static int kq_fd = 0;
static int stop_server = 0;
static int server_socket = 0;
static set* server_files = NULL;
static char* temp_directory = NULL;
static dictionary* connections = NULL;
static struct kevent* events_array = NULL;

connection_t* connection_init(int client_fd) {
    connection_t* this = malloc(sizeof(connection_t));
    this->state = CS_CLIENT_CONNECTED;
    this->parsed_num_bytes = 0;
    this->bytes_to_transmit = 0;
    this->bytes_transmitted = 0;
    this->v = -1;

    this->buf = calloc(BUFFER_SIZE, sizeof(char));
    this->buf_end = 0;
    this->buf_ptr = 0;

    this->filename = NULL;
    this->f = NULL;

    this->client_fd = client_fd;
    dictionary_set(connections, &client_fd, this);
    
    struct kevent client_event;
    EV_SET(&client_event, client_fd, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, NULL);

    if ( kevent(kq_fd, &client_event, 1, NULL, 0, NULL) == -1 ) 
        err(EXIT_FAILURE, "kevent register");
    
    if (client_event.flags & EV_ERROR)
        errx(EXIT_FAILURE, "Event error: %s", strerror(client_event.data));

    return this;
}

void connection_destroy(connection_t* this) {
    if ( this->filename ) 
        free(this->filename);

    if ( this->buf )
        free(this->buf);

    if ( this->f )
        fclose(this->f);

    dictionary_remove(connections, &this->client_fd);
    close(this->client_fd);
    free(this);
}

char* make_path(char* filename) {
    char* ret = calloc(1, strlen(filename) + 8); 
    strcpy(ret, temp_directory);
    strcat(ret, "/");
    strcat(ret, filename);

    return ret;  // path = directory + '/' + filename + '\0'
}

void delete_file_from_server(char* filename) {
    if ( set_contains(server_files, filename) ) // some defensive programming...
        set_remove(server_files, filename);

    char* path = make_path(filename);
    if ( unlink(path) != 0 )
        perror("unlink");

    free(path);
}

void make_socket_non_blocking(int fd) {
    // adapted from: https://github.com/eliben/code-for-blog/blob/master/2017/async-socket-server/utils.c
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        err(EXIT_FAILURE, "fcntl F_GETFL");
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) 
        err(EXIT_FAILURE, "fcntl F_SETFL O_NONBLOCK");
}

void setup_server_socket(char* port) {
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

    // struct sockaddr_in addr;
    // memset(&addr, 0, sizeof(addr));
    // addr.sin_family = AF_INET;
    // addr.sin_port = htons(atoi(port));
    // addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // printf("%d %d\n", htons(atoi(port)), atoi(port));

    // bind(server_socket, (struct sockaddr*) &addr, sizeof(addr));

    if ( listen(server_socket, SOMAXCONN) ) 
        err(EXIT_FAILURE, "listen");

    make_socket_non_blocking(server_socket);
}

void setup_server_resources(void) {
    events_array = calloc(MAX_FILE_DESCRIPTORS, sizeof(struct kevent));
    connections = int_to_shallow_dictionary_create();
    server_files = string_set_create();
    
    temp_directory = strdup("XXXXXX");
    mkdtemp(temp_directory);
    LOG("Storing files at '%s'", temp_directory);

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

void cleanup_server(void) {
    LOG("Exiting server...");
    if ( connections ) {
        vector* values = dictionary_values(connections);
        for (size_t i = 0; i < vector_size(values); ++i)
            connection_destroy((connection_t*) vector_get(values, i));

        dictionary_destroy(connections);
        vector_destroy(values);
    }

    if ( server_files ) {
        vector* filenames = set_elements(server_files);
        for (size_t i = 0; i < vector_size(filenames); ++i) {
            char* f = strdup((char*) vector_get(filenames, i));
            delete_file_from_server(f); free(f);
        }

        vector_destroy(filenames);
        set_destroy(server_files);
    }

    if ( temp_directory ) {
        if ( rmdir(temp_directory) != 0 )
            perror("rmdir");

        free(temp_directory);
    }

    if ( events_array ) 
        free(events_array);
   
    close(server_socket);
}

int handle_new_client(void) {
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

        // LOG("\n(fd=%d) Accepted new connection", client_fd);
        make_socket_non_blocking(client_fd);
        connection_init(client_fd);

        char* client_addr_str = inet_ntoa(client_addr.sin_addr);
        uint16_t client_port = htons(client_addr.sin_port);
        print_client_connected(client_addr_str, client_port);

        return client_fd;
    }
}

void handle_sigint(int signal) { (void)signal; stop_server = 1; }

void handle_sigpipe(int signal) { (void)signal; }

int main(int argc, char** argv) {
    if (argc != 2) {
        errx(EXIT_FAILURE, "./server <port>");
    }

    atexit(cleanup_server);
    setup_server_socket(argv[1]);
    print_server_details(argv[1]);
    setup_server_resources();
    
    kq_fd = kqueue();
    if (kq_fd == -1)
        err(EXIT_FAILURE, "kqueue");

    struct kevent accept_event; 
    EV_SET(&accept_event, server_socket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);

    if (kevent(kq_fd, &accept_event, 1, NULL, 0, NULL) == -1) 
        err(EXIT_FAILURE, "kevent register");
    if (accept_event.flags & EV_ERROR)
        errx(EXIT_FAILURE, "Event error: %s", strerror(accept_event.data));

    int num_events = 0;
    while ( !stop_server ) {
        num_events = kevent(kq_fd, 0, 0, events_array, MAX_FILE_DESCRIPTORS, 0);
        if ( num_events == -1 )  { break; }

        for(int i = 0 ; i < num_events; i++) {
            int fd = events_array[i].ident;
            
            if (fd == server_socket) { 
                // we have a new connection to the server
                handle_new_client();
            } else if (fd > 0) {
                // connection_destroy(dictionary_get(connections, &fd));
                // handle_client(events_array[i].ident, events_array[i].flags);
            }
        }
    }

    exit(0);
}
