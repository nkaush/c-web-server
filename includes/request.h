#pragma once
#include "libs/dictionary.h"

typedef enum _verb { 
    V_GET,
    V_HEAD,
    V_POST,
    V_PUT,
    V_DELETE,
    V_CONNECT,
    V_OPTIONS,
    V_TRACE, 
    V_UNKNOWN = -1
} verb;

typedef struct _request {
    verb request_method; 
    char* path;
    dictionary* headers; // a dictionary of (char*) -> (char*) 
    dictionary* params;  // a dictionary of (char*) -> (char*) 
    char* body;
} request_t;