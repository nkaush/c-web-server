#pragma once
#include <inttypes.h>
#include <time.h>
#include "internals/io_utils.h"
#include "response.h"
#include "request.h"

typedef struct _connection_initializer {
    char* client_address;
    int client_fd;
    uint16_t client_port;
} connection_initializer_t;

// This enum indicates the state of a connection.
typedef enum _connection_state {
    CS_CLIENT_CONNECTED,
    CS_METHOD_PARSED,
    CS_URL_PARSED,
    CS_REQUEST_PARSED,
    CS_HEADERS_PARSED,
    CS_REQUEST_RECEIVED,
    CS_WRITING_RESPONSE_HEADER,
    CS_WRITING_RESPONSE_BODY
} connection_state;

// This data structure keeps track of a connection to a client.
typedef struct _connection {
    request_t* request;
    response_t* response;
    char* client_address;
    char* buf;
    size_t buf_size;
    size_t body_bytes_to_transmit;
    size_t body_bytes_transmitted;
    size_t body_bytes_to_receive;
    size_t body_bytes_received;
    connection_state state;    
    int client_fd;
    int buf_end;
    int buf_ptr;
    struct timespec time_connected;
    struct timespec time_received;
    struct timespec time_begin_send;
    uint16_t client_port; 
    uint8_t flags;
} connection_t;

// Initialize a connection data structure and save to the global connections 
// dictionary under the specified client file descriptor.
void* connection_init(void* ptr);

// Destroy all resources used to keep track of this connection instance.
void connection_destroy(void* ptr);

// Read bytes from a file descriptor and save to the connection buffer. 
ssize_t connection_read(connection_t* conn, size_t event_data);

void connection_shift_buffer(connection_t* conn);

void connection_resize_local_buffer(connection_t* conn, size_t buffer_size);

void connection_resize_sock_send_buf(connection_t* conn, size_t buffer_size);

void connection_resize_sock_rcv_buf(connection_t* conn, size_t buffer_size);

// Attempt to parse the http_method in a header received from a client.
void connection_try_parse_verb(connection_t* conn);

// Attempt to parse the header received from a client.
void connection_try_parse_header(connection_t* conn);

// Attempt to parse the url received from a client.
void connection_try_parse_url(connection_t* conn);

// Attempt to parse the request protocol received from a client.
void connection_try_parse_protocol(connection_t* conn);

// Attempt to parse the request headers received from a client.
void connection_try_parse_headers(connection_t* conn);

void connection_read_request_body(connection_t* conn);

void connection_write_response_header(connection_t* connection);

int connection_try_send_response_body(connection_t* conn, size_t max_receivable);