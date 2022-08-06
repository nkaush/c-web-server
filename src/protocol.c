#include "protocol.h"
#include "libs/dictionary.h"
#include "format.h"

static const char* HTTP_METHOD_STRINGS[NUM_HTTP_METHODS] = {
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "CONNECT",
    "OPTIONS",
    "TRACE"
};

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

static dictionary* status_code_to_string = NULL;

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

const char* http_status_to_string(http_status status) {
    if ( !status_code_to_string )
        load_status_code_dictionary();

    if ( dictionary_contains(status_code_to_string, &status) )
        return (const char*) dictionary_get(status_code_to_string, &status);

    WARN("Unknown http_status enum variant: %d", status);
    return "";
}

const char* http_method_to_string(http_method method) {
    if ( method >= 0 && method < NUM_HTTP_METHODS ) {
        return HTTP_METHOD_STRINGS[method];
    }

    WARN("Unknown http_method enum variant: %d", method);
    return "";
}

// Adapted from https://stackoverflow.com/a/30895866
int url_decode(char* out, const char* in) {
    static const char tbl[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    char c, v1, v2, *beg=out;
    if(in != NULL) {
        while((c=*in++) != '\0') {
            if(c == '%') {
                if((v1=tbl[(unsigned char)*in++])<0 || 
                   (v2=tbl[(unsigned char)*in++])<0) {
                    *beg = '\0';
                    return -1;
                }
                c = (v1<<4)|v2;
            }
            *out++ = c;
        }
    }
    *out = '\0';
    return 0;
}