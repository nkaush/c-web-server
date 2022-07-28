#pragma once
#include <stdio.h>
#include <time.h>
#include "common.h"
#include "request.h"

// This enum indicates the state of a connection.
typedef enum _connection_state {
    CS_CLIENT_CONNECTED,
    CS_VERB_PARSED,
    CS_REQUEST_PARSED,
    CS_HEADERS_PARSED,
    CS_REQUEST_RECEIVED,
    CS_WRITING_RESPONSE
} connection_state;

// This data structure keeps track of a connection to a client.
typedef struct _connection {
    struct timespec time_received;
    char* buf;
    connection_state state;
    size_t bytes_to_transmit;
    size_t bytes_transmitted;
    int parsed_num_bytes;
    char* filename;
    int client_fd;
    int buf_end;
    int buf_ptr;
    verb v;
} connection_t;

// Initialize a connection data structure and save to the global connections 
// dictionary under the specified client file descriptor.
void* connection_init(void* ptr);

// Destroy all resources used to keep track of this connection instance.
void connection_destroy(void* ptr);

// Read bytes from a file descriptor and save to the connection buffer. 
void connection_read(connection_t* conn);

void connection_shift_buffer(connection_t* conn);

// Attempt to parse the verb in a header received from a client.
void connection_try_parse_verb(connection_t* conn);

// Attempt to parse the header received from a client.
void connection_try_parse_header(connection_t* conn);