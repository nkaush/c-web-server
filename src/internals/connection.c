#include "internals/connection.h"
#include "internals/format.h"
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <err.h>

#define REQUEST_BODY_LENGTH_PARSED 0x01
#define RESPONSE_BODY_LENGTH_PARSED 0x02

#define SET_REQUEST_BODY_LENGTH_PARSED(connection) \
    do { connection->flags |= REQUEST_BODY_LENGTH_PARSED; } while (0)
#define SET_RESPONSE_BODY_LENGTH_PARSED(connection) \
    do { connection->flags |= RESPONSE_BODY_LENGTH_PARSED; } while (0)

#define WAS_REQUEST_BODY_LENGTH_PARSED(connection) \
    (connection->flags & REQUEST_BODY_LENGTH_PARSED)
#define WAS_RESPONSE_BODY_LENGTH_PARSED(connection) \
    (connection->flags & RESPONSE_BODY_LENGTH_PARSED)

#define DEFAULT_RCV_BUFFER_SIZE (1UL << 13UL)
#define MAX_RCV_BUFFER_SIZE (1UL << 23UL)
#define MIN_RCV_CLKS 10

#define DEFAULT_SND_BUFFER_SIZE (1UL << 16UL)
#define MAX_SND_BUFFER_SIZE (1UL << 18UL)
#define MIN_SND_CLKS 10

#define MAX_BUFFER_SIZE MAX_RCV_BUFFER_SIZE

static char* CONTENT_LENGTH_HEADER_KEY = "Content-Length";

void* connection_init(void* ptr) {
    connection_initializer_t* c = (connection_initializer_t*) ptr;

    connection_t* this = malloc(sizeof(connection_t));
    this->state = CS_CLIENT_CONNECTED;
    clock_gettime(CLOCK_REALTIME, &this->time_connected);

    this->buf = calloc(DEFAULT_RCV_BUFFER_SIZE, sizeof(char));
    this->buf_size = DEFAULT_RCV_BUFFER_SIZE;
    this->buf_end = 0;
    this->buf_ptr = 0;

    this->body_bytes_to_transmit = 0;
    this->body_bytes_transmitted = 0;
    this->body_bytes_to_receive = 0;
    this->body_bytes_received = 0;
    this->flags = 0;

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

    WARN("destroy connection on fd=%d", this->client_fd);
    close(this->client_fd);
    free(this);
}

ssize_t connection_read(connection_t* conn, size_t event_data) {
    size_t to_read = MIN(event_data, conn->buf_size - conn->buf_end);
    ssize_t bytes_read = 
        read_all_from_socket(conn->client_fd, conn->buf + conn->buf_end, to_read);

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
    conn->buf[conn->buf_end] = '\0';
    conn->buf_ptr = 0;
}

void connection_resize_local_buffer(connection_t* conn, size_t buffer_size) {
    if ( conn->buf_size < buffer_size ) {
        buffer_size = MIN(MAX_BUFFER_SIZE, buffer_size);
        LOG("resize local buffer to %zu", buffer_size);
        conn->buf_size = buffer_size;
        conn->buf = realloc(conn->buf, conn->buf_size);
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
    /// @todo return 413 if headers are too long (i.e. no \r\n\r\n)
    static const char* HEADER_SEP = ": ";
    static const char* EMPTY_HEADER_KEY = "";

    char* header_str = conn->buf + conn->buf_ptr;
    char* token = NULL;

    while ( ( token = strsep(&header_str, CRLF) ) ) {
        // call strsep again to move to next token since CRLF is 2 chars long
        strsep(&header_str, CRLF); 

        // call strsep twice to move to next token since HEADER_SEP is 2 chars
        char* key = strsep(&token, HEADER_SEP);
        strsep(&token, HEADER_SEP);

        // end parsing if we see \r\n\r\n
        if ( !strcmp(key, EMPTY_HEADER_KEY) ) {
            conn->buf_ptr = (key - conn->buf) + 2;
            break;
        }

        if ( token ) {
            dictionary_set(conn->request->headers, key, token);
        } 
    }

    conn->state = CS_HEADERS_PARSED;
}

void allocate_buffer_for_request(connection_t* conn) {
    size_t target_size = conn->body_bytes_to_receive / MIN_RCV_CLKS;
    size_t new_buf_len = MIN(MAX_RCV_BUFFER_SIZE, target_size);
    connection_resize_local_buffer(conn, new_buf_len);
}

void connection_read_request_body(connection_t* conn) {
    /// @todo send error if not put or post and has request body
    request_t* request = conn->request;

    int is_put_or_post = request->method == HTTP_PUT;
    is_put_or_post |= request->method == HTTP_POST;

    if (is_put_or_post && !WAS_REQUEST_BODY_LENGTH_PARSED(conn)) {
        SET_REQUEST_BODY_LENGTH_PARSED(conn);

        if ( !dictionary_contains(request->headers, CONTENT_LENGTH_HEADER_KEY) ) {
            conn->state = CS_WRITING_RESPONSE_HEADER;
            conn->response = response_length_required();
            return;
        }

        sscanf(
            dictionary_get(request->headers, CONTENT_LENGTH_HEADER_KEY),
            "%zu", &conn->body_bytes_to_receive
        );

        if ( conn->body_bytes_to_receive > MAX_RCV_BUFFER_SIZE ) {
            request_init_tmp_file_body(request, conn->body_bytes_to_receive);
            allocate_buffer_for_request(conn);
        } else {
            request_init_str_body(request, conn->body_bytes_to_receive);
        }
    }
    
    if ( is_put_or_post ) {
        request_read_body(conn->request, conn->buf, conn->buf_end);
        conn->body_bytes_received += conn->buf_end;
        conn->buf_end = 0;
        conn->buf_ptr = 0;
    }

    size_t byte_diff = conn->body_bytes_to_receive - conn->body_bytes_received;
    if ( (is_put_or_post && byte_diff == 0) || !is_put_or_post ) {
        conn->state = CS_REQUEST_RECEIVED;
        clock_gettime(CLOCK_REALTIME, &conn->time_received);
    }
}

int format_response_header(response_t* response, char** buffer) {
    static const char* HEADER_FMT = "%s: %s\r\n";
    static const char* RESPONSE_HEADER_FMT = "HTTP/1.0 %d %s\r\n%s\r\n";

    vector* defined_header_keys = dictionary_keys(response->headers);
    vector* formatted_headers = string_vector_create();
    int allocated_space = 1; // allow for NUL-byte at the end

    for (size_t i = 0; i < vector_size(defined_header_keys); ++i) {
        char* key = vector_get(defined_header_keys, i);
        char* value = dictionary_get(response->headers, key);
        char* buffer = NULL;
        allocated_space += asprintf(&buffer, HEADER_FMT, key, value);

        vector_push_back(formatted_headers, buffer);
        free(buffer);
    }

    char* joined_headers = calloc(1, allocated_space);
    for (size_t i = 0; i < vector_size(formatted_headers); ++i)
        strcat(joined_headers, vector_get(formatted_headers, i));
    
    strcat(joined_headers, "\0");

    const char* status_str = http_status_to_string(response->status);
    int ret = asprintf(
        buffer, RESPONSE_HEADER_FMT, 
        response->status, status_str, joined_headers
    );
    
    vector_destroy(defined_header_keys);
    vector_destroy(formatted_headers);
    free(joined_headers);

    return ret;
}

void connection_write_response_header(connection_t* connection) {
    char* header_str = NULL;
    int header_len = format_response_header(connection->response, &header_str);
    
    write_all_to_socket(connection->client_fd, header_str, header_len);
    free(header_str);
    
    connection->state = CS_WRITING_RESPONSE_BODY;
}

void allocate_buffer_for_response(connection_t* conn) {
    size_t target_size = conn->body_bytes_to_transmit / MIN_SND_CLKS;
    size_t new_buf_len = MIN(MAX_SND_BUFFER_SIZE, target_size);

    connection_resize_local_buffer(conn, new_buf_len);

    new_buf_len *= 2;
    size_t snd_buffer_size = socket_snd_buf_size(conn->client_fd);

    if ( new_buf_len > snd_buffer_size ) 
        connection_resize_sock_send_buf(conn, new_buf_len);
}

// @return -1 if there was an error, 1 if the request is ongoing, 0 if the request is complete
int connection_try_send_response_body(connection_t* conn, size_t max_receivable) {
    /// @todo handle RT_EMPTY
    response_t* response = conn->response;

    if ( !WAS_RESPONSE_BODY_LENGTH_PARSED(conn) ) {
        sscanf(
            dictionary_get(response->headers, CONTENT_LENGTH_HEADER_KEY), 
            "%zu", &conn->body_bytes_to_transmit
        );

        allocate_buffer_for_response(conn);
        SET_RESPONSE_BODY_LENGTH_PARSED(conn);
        clock_gettime(CLOCK_REALTIME, &conn->time_begin_send);
    }

    size_t to_send = conn->body_bytes_to_transmit - conn->body_bytes_transmitted;
    to_send = MIN(to_send, MIN(conn->buf_size, max_receivable));

    if ( response->rt == RT_FILE ) {
        fread(conn->buf, sizeof(char), to_send, response->body_content.file);
    } else if ( response->rt == RT_STRING ) {
        const char* snd_buf = 
            response->body_content.body + conn->body_bytes_transmitted;
        memcpy(conn->buf, snd_buf, to_send);
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
    } 

    conn->body_bytes_transmitted += return_code;
    if ( conn->body_bytes_transmitted == conn->body_bytes_to_transmit )
        return 0;
    
    return 1;
}
