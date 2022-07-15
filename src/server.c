#include "utils/dictionary.h"
#include "utils/callbacks.h"
#include "utils/set.h"
#include "connection.h"
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
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <err.h>

#define MAX_FILE_DESCRIPTORS 1024

static int kq_fd = 0;
static int stop_server = 0;
static int server_socket = 0;
static set* server_files = NULL;
static char* temp_directory = NULL;
static dictionary* connections = NULL;
static struct kevent* events_array = NULL;

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

    if ( listen(server_socket, SOMAXCONN) ) 
        err(EXIT_FAILURE, "listen");

    make_socket_non_blocking(server_socket);
}

void setup_server_resources(void) {
    events_array = calloc(MAX_FILE_DESCRIPTORS, sizeof(struct kevent));
    connections = dictionary_create(
        int_hash_function, int_compare, 
        int_copy_constructor, int_destructor,
        connection_init, connection_destroy
    );

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
    if ( connections )
        dictionary_destroy(connections);

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

void handle_client(int client_fd, struct kevent* event) {
    connection_t* conn = dictionary_get(connections, &client_fd);
    connection_read(conn);

    if ( conn->state == CS_CLIENT_CONNECTED ) {
        try_parse_verb(conn);
    }
    
    if ( conn-> state == CS_VERB_PARSED ) {

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

void try_parse_verb(connection_t* conn) {
    size_t idx_space = 0;
    for (size_t i = 3; i < 8; ++i) { // look for a space between index 3 and 8
        if ( conn->buf[i] == ' ' ) {
            idx_space = i; break;
        }
    }

    if ( !idx_space ) {
        conn->state = CS_REQUEST_RECEIVED;
        conn->v = V_UNKNOWN;
        return;
    }

    conn->buf[idx_space] = '\0';
    size_t verb_hash = string_hash_function(conn->buf);
    // hash the string value to avoid many calls to strncmp
    // GET     --> 193456677
    // HEAD    --> 6384105719
    // POST    --> 6384404715
    // PUT     --> 193467006
    // DELETE  --> 6952134985656
    // CONNECT --> 229419557091567
    // OPTIONS --> 229435100789681
    // TRACE   --> 210690186996

    switch ( verb_hash ) {
        case 193456677UL:
            conn->v = V_GET; break;
        case 6384105719UL:
            conn->v = V_HEAD; break;
        case 6384404715UL:
            conn->v = V_POST; break;
        case 193467006UL:
            conn->v = V_PUT; break;
        case 6952134985656UL:
            conn->v = V_DELETE; break;
        case 229419557091567UL:
            conn->v = V_CONNECT; break;
        case 229435100789681UL:
            conn->v = V_OPTIONS; break;
        case 210690186996UL:
            conn->v = V_TRACE; break;
        default:
            conn->v = V_UNKNOWN; conn->state = CS_REQUEST_RECEIVED; return;
    }

    conn->state = CS_VERB_PARSED;
    conn->buf_ptr = idx_space + 1;
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
            events_array[i].
            if (fd == server_socket) { 
                // we have a new connection to the server
                handle_new_client();
            } else if (fd > 0) {
                handle_client(fd, events_array + i);

                
                // connection_destroy(dictionary_get(connections, &fd));
                // handle_client(events_array[i].ident, events_array[i].flags);
            }
        }
    }

    exit(0);
}
