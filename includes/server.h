#pragma once
#include <stdio.h>
#include "common.h"

// Use the fcntl interface to make the specified socket non-blocking.
void make_socket_non_blocking(int fd);

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
