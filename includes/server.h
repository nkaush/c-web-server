#pragma once
#include <stdio.h>
#include "common.h"

#define BUFFER_SIZE 4096

// This enum indicates the state of a connection.
typedef enum _connection_state {
    CS_CLIENT_CONNECTED,
    CS_HEADER_PARSED,
    CS_REQUEST_RECEIVED,
    CS_WRITING_RESPONSE
} connection_state;

// This data structure keeps track of a connection to a client.
typedef struct _connection {
    char* buf;
    connection_state state;
    size_t bytes_to_transmit;
    size_t bytes_transmitted;
    int parsed_num_bytes;
    char* filename;
    int client_fd;
    int buf_end;
    int buf_ptr;
    FILE* f;
    verb v;
} connection_t;

// Initialize a connection data structure and save to the global connections 
// dictionary under the specified client file descriptor.
connection_t* connection_init(int client_fd);

// Destroy all resources used to keep track of this connection instance.
void connection_destroy(connection_t* this);

// Read bytes from a file descriptor and save to the connection buffer. 
void connection_read(connection_t* conn);

// Check if the buffer of characters read contains the specified character.
ssize_t connection_buf_index_of_char(connection_t* this, char c);

// Use the fcntl interface to make the specified socket non-blocking.
void make_socket_non_blocking(int fd);

// Construct a path for the specified file using the server files directory.
// Allocates data on the heap for the returned path.l
char* make_path(char* filename);

// Deletes the specified file from the server.
void delete_file_from_server(char* filename);

// Configure the server socket.
void setup_server_socket(char* port);

// Configure the server's resources.
void setup_server_resources(void);

// Cleanup the resources used by the server. Called on program exit.
void cleanup_server(void);

// Gracefully capture SIGINT and stop the server.
void handle_sigint(int signal);

// Gracefully capture SIGPIPE and do nothing.
void handle_sigpipe(int signal);

// Handle an kqueue event triggered from a client requesting to connect.
int handle_new_client(void);

// Handle an kqueue event from a client connection.
void handle_client(int client_fd, int event);

// Handle a PUT request from a client connection. Writes bytes sent to a file.
void handle_put_request(connection_t* conn, int kqueue_event);

// Respond to a PUT request from a client connection.
void handle_put_response(connection_t* conn);

// Respond to a LIST request from a client connection.
void handle_list_response(connection_t* conn);

// Respond to a GET request from a client connection.
void handle_get_response(connection_t* conn);

// Respond to a DELETE request from a client connection.
void handle_delete_response(connection_t* conn);

// Respond to an unknown request from a client connection.
void handle_unknown_response(connection_t* conn);

// Attempt to parse a header received from a client.
int try_parse_header(connection_t* conn);
