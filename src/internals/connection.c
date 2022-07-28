#include "internals/connection.h"
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
    this->request_method = -1;

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

void connection_try_parse_verb(connection_t* conn) {
    size_t idx_space = 0;
    for (size_t i = 3; i < 8; ++i) { // look for a space between index 3 and 8
        if ( conn->buf[i] == ' ' ) {
            idx_space = i; break;
        }
    }

    if ( !idx_space ) {
        conn->state = CS_REQUEST_RECEIVED;
        conn->request_method = V_UNKNOWN;
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
            conn->request_method = V_GET; break;
        case 6384105719UL:
            conn->request_method = V_HEAD; break;
        case 6384404715UL:
            conn->request_method = V_POST; break;
        case 193467006UL:
            conn->request_method = V_PUT; break;
        case 6952134985656UL:
            conn->request_method = V_DELETE; break;
        case 229419557091567UL:
            conn->request_method = V_CONNECT; break;
        case 229435100789681UL:
            conn->request_method = V_OPTIONS; break;
        case 210690186996UL:
            conn->request_method = V_TRACE; break;
        default:
            conn->request_method = V_UNKNOWN; conn->state = CS_REQUEST_RECEIVED; return;
    }

    conn->state = CS_VERB_PARSED;
    conn->buf_ptr = idx_space + 1;
}