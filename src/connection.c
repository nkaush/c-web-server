#include "connection.h"
#include <stdlib.h>
#include <unistd.h>

void* connection_init(void* ptr) {
    int client_fd = *((int*) ptr);
    connection_t* this = malloc(sizeof(connection_t));
    this->state = CS_CLIENT_CONNECTED;
    this->parsed_num_bytes = 0;
    this->bytes_to_transmit = 0;
    this->bytes_transmitted = 0;
    this->v = -1;

    this->buf = calloc(BUFFER_SIZE, sizeof(char));
    this->buf_end = 0;
    this->buf_ptr = 0;

    this->client_fd = client_fd;

    return this;
}

void connection_destroy(void* ptr) {
    connection_t* this = ptr;
    if ( this->buf )
        free(this->buf);

    close(this->client_fd);
    free(this);
}