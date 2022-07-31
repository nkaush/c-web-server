#pragma once
#include "libs/dictionary.h"
#include "protocol.h"

// This struct contains information about a client's request
typedef struct _request {
    http_method method; 
    dictionary* headers; // a dictionary of (char*) -> (char*) 
    dictionary* params;  // a dictionary of (char*) -> (char*) 
    char* protocol;
    char* path;
    char* body;
} request_t;

// Construct a request_t struct using the specified HTTP request method
request_t* request_create(http_method method);

// Destroy the passed request_t struct
void request_destroy(request_t* request);

void request_parse_query_params(request_t* request);