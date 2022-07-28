#pragma once
#include "libs/dictionary.h"

#define NUM_HTTP_METHODS 8

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

typedef struct _request {
    http_method request_method; 
    dictionary* headers; // a dictionary of (char*) -> (char*) 
    dictionary* params;  // a dictionary of (char*) -> (char*) 
    char* path;
    char* body;
} request_t;

char* http_method_to_string(http_method method);