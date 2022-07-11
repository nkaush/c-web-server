#pragma once
#include "common.h"
#define BUFFER_SIZE 4096

// This data structure keeps track of a connection to a client.
typedef struct _connection {
    char* buf;
    connection_state state;
    size_t bytes_to_transmit;
    size_t bytes_transmitted;
    int parsed_num_bytes;
    int client_fd;
    int buf_end;
    int buf_ptr;
    FILE* f;
    verb v;
} connection_t;

// Initialize a connection data structure and save to the global connections 
// dictionary under the specified client file descriptor.
void* connection_init(void* ptr);

// Destroy all resources used to keep track of this connection instance.
void connection_destroy(void* ptr);

// Read bytes from a file descriptor and save to the connection buffer. 
void connection_read(connection_t* conn);

// Check if the buffer of characters read contains the specified character.
ssize_t connection_buf_index_of_char(connection_t* this, char c);