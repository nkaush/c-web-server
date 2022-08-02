#include "internals/connection.h"
#include "internals/format.h"
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>

// #define BUFFER_SIZE 8192
// #define BUFFER_SIZE 16384
// #define BUFFER_SIZE 32768
#define DEFAULT_BUFFER_SIZE 65536

void* connection_init(void* ptr) {
    connection_initializer_t* c = (connection_initializer_t*) ptr;

    connection_t* this = malloc(sizeof(connection_t));
    this->state = CS_CLIENT_CONNECTED;
    clock_gettime(CLOCK_REALTIME, &this->time_received);

    this->buf = calloc(DEFAULT_BUFFER_SIZE, sizeof(char));
    this->buffer_size = DEFAULT_BUFFER_SIZE;
    this->buf_end = 0;
    this->buf_ptr = 0;

    this->bytes_to_transmit = 0;
    this->bytes_transmitted = 0;

    this->response = NULL;
    this->request = NULL;

    this->client_address = strdup(c->client_address);
    this->client_port = c->client_port;
    this->client_fd = c->client_fd;

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

    if ( this->client_address )
        free(this->client_address);

    close(this->client_fd);
    free(this);
}

ssize_t connection_read(connection_t* conn) {
    ssize_t bytes_read = read(
        conn->client_fd, conn->buf + conn->buf_end, conn->buffer_size - conn->buf_end
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

    conn->buf_end = bytes_read;
    return bytes_read;
}

void connection_shift_buffer(connection_t* conn) {
    memmove(conn->buf, conn->buf + conn->buf_ptr, conn->buf_end - conn->buf_ptr);
    conn->buf_end -= conn->buf_ptr;
    conn->buf_ptr = 0;
}

void connection_resize_local_buffer(connection_t* conn, size_t buffer_size) {
    if ( conn->buffer_size < buffer_size ) {
        LOG("resize local buffer to %zu", buffer_size);
        conn->buffer_size = buffer_size;
        conn->buf = realloc(conn->buf, conn->buffer_size);
    }
}

void connection_resize_sock_rcv_buf(connection_t* conn, size_t buffer_size) {
    LOG("resize rcv buffer to %zu", buffer_size);
    setsockopt(conn->client_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
}

void connection_resize_sock_send_buf(connection_t* conn, size_t buffer_size) {
    LOG("resize send buffer to %zu", buffer_size);
    setsockopt(conn->client_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
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
        conn->state = CS_WRITING_RESPONSE_HEADER;
        conn->response = response_uri_too_long();
        return;
    }

    *idx_space = '\0';
    conn->request->path = strdup(url_start);
    conn->state = CS_URL_PARSED;
    conn->buf_ptr += length + 1;

    request_parse_query_params(conn->request);
}

void connection_try_parse_protocol(connection_t* conn) {
    /// @todo maybe some kind of validation on the protocol?
    char* protocol_start = conn->buf + conn->buf_ptr;
    char* idx_space = strstr(protocol_start, CRLF);

    // DO NOT add 1 since we are excluding the space from the length
    size_t length = (size_t) idx_space - (size_t) protocol_start;

    // Set the CRLF pattern to NULL so it is not included in the protocol
    *idx_space++ = '\0';
    conn->request->protocol = strdup(protocol_start);
    conn->state = CS_REQUEST_PARSED;
    conn->buf_ptr += length + 2; // add 1 to skip the \n at the end of CRLF
}

void connection_try_parse_headers(connection_t* conn) {
    static const char* HEADER_SEP = ": ";
    static const char* EMPTY_HEADER_KEY = "";

    char* header_str = conn->buf + conn->buf_ptr;
    int was_prev_empty = 0;

    char* token = NULL;
    while ( ( token = strsep(&header_str, CRLF) ) ) {
        // call strsep again to move to next token since CRLF is 2 chars long
        strsep(&header_str, CRLF); 

        // call strsep twice to move to next token since HEADER_SEP is 2 chars
        char* key = strsep(&token, HEADER_SEP);
        strsep(&token, HEADER_SEP);

        // end parsing if we see 2 empty lines
        if ( !strcmp(key, EMPTY_HEADER_KEY) ) {
            if ( !was_prev_empty ) { was_prev_empty = 1; continue; }
            else break;
        }  

        if ( token )
            dictionary_set(conn->request->headers, key, token);
    }

    conn->state = CS_HEADERS_PARSED;
}

// @return -1 if there was an error, 1 if the request is ongoing, 0 if the request is complete
int connection_try_send_response_body(connection_t* conn, size_t max_receivable) {
    /// @todo handle RT_EMPTY
    response_t* response = conn->response;
    size_t to_send = MIN(conn->buffer_size, conn->bytes_to_transmit - conn->bytes_transmitted);
    to_send = MIN(to_send, max_receivable);

    if ( response->rt == RT_FILE ) {
        fread(conn->buf, sizeof(char), to_send, response->body_content.file);
    } else if ( response->rt == RT_STRING ) {
        memcpy(conn->buf, conn->response->body_content.body, to_send);
    }
    
    ssize_t return_code = 
        write_all_to_socket(conn->client_fd, conn->buf, to_send);
    
    if (return_code < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG("(fd=%d) write_all_to_socket() returned with code -1 and EAGAIN/EWOULDBLOCK", conn->client_fd);
        } else {
            LOG("(fd=%d) write_all_to_socket() returned with code -1", conn->client_fd);
            connection_destroy(conn);
        }
        return -1;
    } else if (return_code == 0) {
        LOG("(fd=%d) write_all_to_socket() returned with code 0", conn->client_fd);
        return 0;
    } else {
        LOG("sent %zd bytes", return_code);
        conn->bytes_transmitted += return_code;
        return 1;
    }
}