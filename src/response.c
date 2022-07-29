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

static dictionary* status_code_to_string = NULL;

static const char* HTTP_STATUS_STRINGS[NUM_HTTP_STATUS_CODES] = {
    "OK",
    "Created",
    "Accepted",
    "No Content",
    "Moved Permanently",
    "Found",
    "See Other",
    "Not Modified",
    "Temporary Redirect",
    "Permanent Redirect",
    "Bad Request",
    "Unauthorized",
    "Forbidden",
    "Not Found",
    "Method Not Allowed",
    "Request Timeout",
    "Length Required",
    "Payload Too Large",
    "URI Too Long",
    "Unsupported Media Type",
    "I'm a teapot",
    "Too Many Requests",
    "Internal Server Error",
    "Not Implemented",
    "Bad Gateway",
    "Service Unavailable",
    "Gateway Timeout",
    "HTTP Version Not Supported",
    "Insufficient Storage"
};

static const http_status HTTP_STATUS_VARIANTS[NUM_HTTP_STATUS_CODES] = {
    STATUS_OK,
    STATUS_CREATED,
    STATUS_ACCEPTED,
    STATUS_NO_CONTENT,
    STATUS_MOVED_PERMANENTLY,
    STATUS_FOUND,
    STATUS_SEE_OTHER,
    STATUS_NOT_MODIFIED,
    STATUS_TEMPORARY_REDIRECT,
    STATUS_PERMANENT_REDIRECT,
    STATUS_BAD_REQUEST,
    STATUS_UNAUTHORIZED,
    STATUS_FORBIDDEN,
    STATUS_NOT_FOUND,
    STATUS_METHOD_NOT_ALLOWED,
    STATUS_REQUEST_TIMEOUT,
    STATUS_LENGTH_REQUIRED,
    STATUS_PAYLOAD_TOO_LARGE,
    STATUS_URI_TOO_LONG,
    STATUS_UNSUPPORTED_MEDIA_TYPE,
    STATUS_IM_A_TEAPOT,
    STATUS_TOO_MANY_REQUESTS,
    STATUS_INTERNAL_SERVER_ERROR,
    STATUS_NOT_IMPLEMENTED,
    STATUS_BAD_GATEWAY,
    STATUS_SERVICE_UNAVAILABLE,
    STATUS_GATEWAY_TIMEOUT,
    STATUS_HTTP_VERSION_NOT_SUPPORTED,
    STATUS_INSUFFICIENT_STORAGE,
};

void cleanup_status_code_to_string_dict(void) {
    dictionary_destroy(status_code_to_string);
}

void load_status_code_dictionary(void) {
    if ( status_code_to_string ) 
        return;

    status_code_to_string = dictionary_create(
        int_hash_function, int_compare, 
        int_copy_constructor, int_destructor, 
        shallow_copy_constructor, shallow_destructor
    );

    for (size_t i = 0; i < NUM_HTTP_STATUS_CODES; ++i) {
        dictionary_set(
            status_code_to_string, 
            (void*) (HTTP_STATUS_VARIANTS + i), 
            (void*) HTTP_STATUS_STRINGS[i]
        );
    }

    atexit(cleanup_status_code_to_string_dict);
}

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
    return response_format_error(msg, STATUS_METHOD_NOT_ALLOWED);
}

response_t* response_resource_not_found(void) {
    static const char* msg = "The requested resource was not found";
    return response_format_error(msg, STATUS_NOT_FOUND);
}

response_t* response_method_not_allowed(void) {
    static const char* msg = "The request method is inappropriate for the requested resource";
    return response_format_error(msg, STATUS_METHOD_NOT_ALLOWED);
}

void response_destroy(response_t* response) {
    dictionary_destroy(response->headers);
    free(response);
}

const char* http_status_to_string(http_status status) {
    if ( !status_code_to_string )
        load_status_code_dictionary();

    if ( dictionary_contains(status_code_to_string, &status) )
        return (const char*) dictionary_get(status_code_to_string, &status);

    WARN("Unknown http_status enum variant: %d", status);
    return "";
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
