#include "connection.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>

#define BUFFER_SIZE 4096

void* connection_init(void* ptr) {
    int client_fd = *((int*) ptr);

    connection_t* this = malloc(sizeof(connection_t));
    this->state = CS_CLIENT_CONNECTED;
    this->parsed_num_bytes = 0;
    this->bytes_to_transmit = 0;
    this->bytes_transmitted = 0;
    this->v = -1;

    clock_gettime(CLOCK_REALTIME, &this->time_received);

    this->buf = calloc(BUFFER_SIZE, sizeof(char));
    this->buf_end = 0;
    this->buf_ptr = 0;

    this->filename = NULL;
    this->client_fd = client_fd;

    return (void*) this;
}

void connection_destroy(void* ptr) {
    connection_t* this = (connection_t*) ptr;

    if ( this->filename ) 
        free(this->filename);

    if ( this->buf )
        free(this->buf);

    close(this->client_fd);
    free(this);
}

void connection_read(connection_t* conn) {
    ssize_t bytes_read = read(
        conn->client_fd, 
        conn->buf + conn->buf_end, 
        BUFFER_SIZE - conn->buf_end
    );

    // adapted from: https://github.com/eliben/code-for-blog/blob/master/2017/async-socket-server/epoll-server.c
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // The socket is not *really* ready for recv; wait until it is.
            return;
        } else {
            err(EXIT_FAILURE, "read");
        }
    }

    conn->buf_end += bytes_read;
}

void connection_shift_buffer(connection_t* conn) {
    memmove(conn->buf, conn->buf + conn->buf_ptr, conn->buf_end - conn->buf_ptr);
    conn->buf_end -= conn->buf_ptr;
    conn->buf_ptr = 0;
}