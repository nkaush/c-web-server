#include "response.h"
#include "format.h"

#include <sys/utsname.h>
#include <sys/stat.h>
#include <time.h>
#include <err.h>

static char server_os[64] = { 0 };

static char* DATE_HEADER_KEY = "Date";
static char* SERVER_HEADER_KEY = "Server";
static char* CONNECTION_HEADER_KEY = "Connection";
static char* CONTENT_TYPE_HEADER_KEY = "Content-Type";
static char* CONTENT_LENGTH_HEADER_KEY = "Content-Length";
static char* LAST_MODIFIED_HEADER_KEY = "Last-Modified";

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
        sprintf(server_os, "kqueue-epoll/0.0.1 (%s %s)", uts.sysname, uts.release);
    }

    dictionary_set(response->headers, DATE_HEADER_KEY, time_buf);
    dictionary_set(response->headers, SERVER_HEADER_KEY, server_os);

    /// @todo make this dynamic
    dictionary_set(response->headers, CONNECTION_HEADER_KEY, "close");

    return response;
}

void response_destroy(response_t* response) {
    dictionary_destroy(response->headers);

    if ( response->rt == RT_FILE )
        fclose(response->body_content.file);
    else if ( response->rt == RT_STRING && response->body_content.body )
        free((void*) response->body_content.body);

    free(response);
}

response_t* response_from_file(http_status status, FILE* file) {
    response_t* response = response_create(status);
    response->body_content.file = file;
    response->rt = RT_FILE;

    struct stat info = { 0 };
    if ( fstat(fileno(response->body_content.file), &info) != 0 )
        err(EXIT_FAILURE, "fstat");

    char time_buf[TIME_BUFFER_SIZE] = { 0 };
#if defined(__APPLE__)
    format_time(time_buf, info.st_mtimespec.tv_sec);
#elif defined(__linux__)
    format_time(time_buf, info.st_mtim.tv_sec);
#endif
    dictionary_set(response->headers, LAST_MODIFIED_HEADER_KEY, time_buf);
    response_set_content_length(response, (size_t) info.st_size);
    
    return response;
}

response_t* response_from_string(http_status status, const char* body) {
    response_t* response = response_create(status);
    response->body_content.body = strdup(body);
    response->rt = RT_STRING;

    response_set_content_length(response, strlen(response->body_content.body)); 

    return response;
}

response_t* response_empty(http_status status) {
    response_t* response = response_create(status);
    response->body_content.body = NULL;
    response->rt = RT_EMPTY;

    response_set_content_length(response, 0);

    return response;
}

response_t* response_format_error(http_status status, const char* msg) {
    char* buf = NULL;
    asprintf(&buf, JSON_ERROR_CONTENT_FMT, msg, status);

    response_t* response = response_from_string(status, buf);
    free(buf);

    return response;
}

response_t* response_malformed_request(request_t* request) {
    (void) request;
    static const char* msg = "The client has issued a malformed or illegal request, and the server was unable to process it";
    return response_format_error(STATUS_BAD_REQUEST, msg);
}

response_t* response_bad_request(request_t* request) {
    (void) request;
    static const char* msg = "The server was unable to process the request";
    return response_format_error(STATUS_BAD_REQUEST, msg);
}

response_t* response_resource_not_found(request_t* request) {
    (void) request;
    static const char* msg = "The requested resource was not found";
    return response_format_error(STATUS_NOT_FOUND, msg);
}

response_t* response_method_not_allowed(request_t* request) {
    (void) request;
    static const char* msg = "The request method is inappropriate for the requested resource";
    return response_format_error(STATUS_METHOD_NOT_ALLOWED, msg);
}

response_t* response_length_required(request_t* request) {
    (void) request;
    static const char* msg = "The Content-Length header is required";
    return response_format_error(STATUS_LENGTH_REQUIRED, msg);
}

response_t* response_uri_too_long(request_t* request) {
    (void) request;
    static const char* msg = "The requested URI is too long";
    return response_format_error(STATUS_URI_TOO_LONG, msg);
}

void response_set_content_type(response_t* response, const char* content_type) {
    dictionary_set(response->headers, CONTENT_TYPE_HEADER_KEY, (void*) content_type);
}

void response_set_content_length(response_t* response, size_t length) {
    char* buf = NULL;
    asprintf(&buf, "%zu", length);
    dictionary_set(response->headers, CONTENT_LENGTH_HEADER_KEY, buf);
    free(buf);
}

void response_set_header(response_t* response, const char* key, const char* value) {
    dictionary_set(response->headers, (void*) key, (void*) value);
}
