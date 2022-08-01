#include "response.h"
#include "internals/format.h"

#include <sys/utsname.h>
#include <time.h>
#include <err.h>

static char server_os[64] = { 0 };

// Format: (message, status code)
static const char* JSON_ERROR_CONTENT_FMT = "{\"message\":\"%s\",\"code\":%d}";

response_t* response_create(http_status status) {
    response_t* response = malloc(sizeof(response_t));
    response->status = status;
    response->headers = string_to_string_dictionary_create();

    char time_buf[TIME_BUFFER_SIZE] = { 0 };
    format_current_time(time_buf);

    if ( !(*server_os) ) {
        struct utsname uts;
        uname(&uts);
        sprintf(server_os, "kqueue-epoll-server/0.0.1 (%s %s)", uts.sysname, uts.release);
    }

    dictionary_set(response->headers, "Date", time_buf);
    dictionary_set(response->headers, "Server", server_os);
    dictionary_set(response->headers, "Connection", "close");

    return response;
}

response_t* response_from_file(http_status status, FILE* file) {
    response_t* response = response_create(status);
    response->body_content.file = file;
    response->rt = RT_FILE;

    /// @todo set the last-modified header to the file last modified time
    
    return response;
}

response_t* response_from_string(http_status status, const char* body) {
    response_t* response = response_create(status);
    response->body_content.body = body;
    response->rt = RT_STRING;

    return response;
}

response_t* response_empty(http_status status) {
    response_t* response = response_create(status);
    response->body_content.body = NULL;
    response->rt = RT_EMPTY;

    return response;
}

response_t* response_format_error(http_status status, const char* msg) {
    response_t* response = response_create(status);
    response->rt = RT_STRING;

    char* buf = NULL;
    asprintf(&buf, JSON_ERROR_CONTENT_FMT, msg, status);
    response->body_content.body = buf;

    return response;
}

response_t* response_bad_request(void) {
    static const char* msg = "The server was unable to process the request";
    return response_format_error(STATUS_BAD_REQUEST, msg);
}

response_t* response_resource_not_found(void) {
    static const char* msg = "The requested resource was not found";
    return response_format_error(STATUS_NOT_FOUND, msg);
}

response_t* response_method_not_allowed(void) {
    static const char* msg = "The request method is inappropriate for the requested resource";
    return response_format_error(STATUS_METHOD_NOT_ALLOWED, msg);
}

response_t* response_uri_too_long(void) {
    static const char* msg = "The requested URI is too long";
    return response_format_error(STATUS_URI_TOO_LONG, msg);
}

void response_destroy(response_t* response) {
    dictionary_destroy(response->headers);
    free(response);
}

void response_set_content_type(response_t* response, const char* content_type) {
    dictionary_set(response->headers, "Content-Type", (void*) content_type);
}

void response_set_content_length(response_t* response, size_t length) {
    char* buf = NULL;
    asprintf(&buf, "%zu", length);
    dictionary_set(response->headers, "Content-Length", buf);
    free(buf);
}

void response_set_header(response_t* response, const char* key, const char* value) {
    dictionary_set(response->headers, (void*) key, (void*) value);
}
