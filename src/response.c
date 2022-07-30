#include "response.h"
#include "internals/format.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <err.h>

// Format: (header_name, header_value)
static const char* HEADER_FMT = "%s: %s\r\n";

// Format: (response_code, response_string, all_headers, response_body)
// Assumes that headers come with \r\n at the end
static const char* RESPONSE_HEADER_FMT = "HTTP/1.0 %d %s\r\n%s\r\n";

// Format: (message, status code)
static const char* JSON_ERROR_CONTENT_FMT = "{\"message\":\"%s\",\"code\":%d}";

response_t* response_create(http_status status) {
    response_t* response = malloc(sizeof(response_t));
    response->status = status;
    response->headers = string_to_string_dictionary_create();

    return response;
}

response_t* response_from_file(http_status status, FILE* file) {
    response_t* response = response_create(status);
    response->body_content.file = file;
    response->rt = RT_FILE;
    
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
    return response_format_error(STATUS_METHOD_NOT_ALLOWED, msg);
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

void infer_content_length(response_t* response) {
    switch ( response->rt ) {
        case RT_FILE: {
            struct stat info = { 0 };
            if ( fstat(fileno(response->body_content.file), &info) != 0 )
                err(EXIT_FAILURE, "fstat");

            response_set_content_length(response, (size_t) info.st_size);
        } break;
        case RT_STRING:
            response_set_content_length(response, strlen(response->body_content.body)); break;
        case RT_EMPTY:
            response_set_content_length(response, 0); break;
    }
}

int response_format_header(response_t* response, char** buffer) {
    char time_buf[TIME_BUFFER_SIZE] = { 0 };
    format_time(time_buf);

    dictionary_set(response->headers, "Date", time_buf);
    dictionary_set(response->headers, "Server", "kqueue-server/0.0.1 (MacOS X)");
    dictionary_set(response->headers, "Connection", "close");
    
    if ( !dictionary_contains(response->headers, "Content-Length") )
        infer_content_length(response);
    
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
    int ret = asprintf(buffer, RESPONSE_HEADER_FMT, response->status, status_str, joined_headers);
    
    vector_destroy(defined_header_keys);
    vector_destroy(formatted_headers);
    free(joined_headers);

    return ret;
}
