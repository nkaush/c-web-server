#pragma once
#include "libs/dictionary.h"

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