#pragma once
#include "libs/dictionary.h"

#define NUM_HTTP_METHODS 8
#define MAX_URL_LENGTH 2048

// This enum indicates the HTTP verb used in a request.
typedef enum _http_method { 
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_CONNECT,
    HTTP_OPTIONS,
    HTTP_TRACE, 
    HTTP_UNKNOWN = -1
} http_method;

// This struct contains information about a client's request
typedef struct _request {
    http_method method; 
    dictionary* headers; // a dictionary of (char*) -> (char*) 
    dictionary* params;  // a dictionary of (char*) -> (char*) 
    char* url_buffer;
    char* protocol;
    char* path;
    char* body;
} request_t;

// Construct a request_t struct using the specified HTTP request method
request_t* request_create(http_method method);

// Destroy the passed request_t struct
void request_destroy(request_t* request);

const char* http_method_to_string(http_method method);