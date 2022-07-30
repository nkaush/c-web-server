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
    clock_gettime(CLOCK_REALTIME, &this->time_received);

    this->buf = calloc(BUFFER_SIZE, sizeof(char));
    this->buf_end = 0;
    this->buf_ptr = 0;

    this->client_fd = client_fd;
    this->response = NULL;
    this->request = NULL;

    return (void*) this;
}

void connection_destroy(void* ptr) {
    connection_t* this = (connection_t*) ptr;

    if ( this->buf )
        free(this->buf);

    if ( this->request )
        request_destroy(this->request);

    if ( this->response )
        response_destroy(this->response);

    close(this->client_fd);
    free(this);
}

ssize_t connection_read(connection_t* conn) {
    ssize_t bytes_read = read(
        conn->client_fd, 
        conn->buf + conn->buf_end, 
        BUFFER_SIZE - conn->buf_end
    );

    // adapted from: https://github.com/eliben/code-for-blog/blob/master/2017/async-socket-server/epoll-server.c
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // The socket is not *really* ready for recv; wait until it is.
            return -1;
        } else {
            err(EXIT_FAILURE, "read");
        }
    }

    conn->buf_end += bytes_read;
    return bytes_read;
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
        conn->request = request_create(HTTP_UNKNOWN);
        return;
    }

    conn->buf[idx_space] = '\0';
    
    // hash the string value and compare against known hash values to avoid many 
    // calls to strncmp since HTTP methods ARE case sensitive
    switch ( string_hash_function(conn->buf) ) {
        case 193456677UL:
            conn->request = request_create(HTTP_GET); break;
        case 6384105719UL:
            conn->request = request_create(HTTP_HEAD); break;
        case 6384404715UL:
            conn->request = request_create(HTTP_POST); break;
        case 193467006UL:
            conn->request = request_create(HTTP_PUT); break;
        case 6952134985656UL:
            conn->request = request_create(HTTP_DELETE); break;
        case 229419557091567UL:
            conn->request = request_create(HTTP_CONNECT); break;
        case 229435100789681UL:
            conn->request = request_create(HTTP_OPTIONS); break;
        case 210690186996UL:
            conn->request = request_create(HTTP_TRACE); break;
        default:
            conn->request = request_create(HTTP_UNKNOWN); conn->state = CS_REQUEST_RECEIVED; return;
    }

    conn->state = CS_METHOD_PARSED;
    conn->buf_ptr = idx_space + 1;
}

void connection_try_parse_url(connection_t* conn) {
    char* url_start = conn->buf + conn->buf_ptr;
    char* idx_space = strstr(url_start, " ");

    // DO NOT add 1 since we are excluding the space from the length
    size_t length = (size_t) idx_space - (size_t) url_start;

    if ( length >= MAX_URL_LENGTH ) {
        conn->state = CS_WRITING_RESPONSE;
        conn->response = response_uri_too_long();
        return;
    }

    *idx_space = '\0';
    conn->request->path = strdup(url_start);
    conn->state = CS_URL_PARSED;
    conn->buf_ptr += length + 1;

    /// @todo parse request params
}

void connection_try_parse_protocol(connection_t* conn) {
    /// @todo maybe some kind of validation on the protocol?
    char* protocol_start = conn->buf + conn->buf_ptr;
    char* idx_space = strstr(protocol_start, "\r\n");

    // DO NOT add 1 since we are excluding the space from the length
    size_t length = (size_t) idx_space - (size_t) protocol_start;

    // Set the CRLF pattern to NULL so it is not included in the protocol
    *idx_space++ = '\0';
    conn->request->protocol = strdup(protocol_start);
    conn->state = CS_REQUEST_PARSED;
    conn->buf_ptr += length + 2; // add 1 to skip the \n at the end of CRLF
}