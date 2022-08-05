#pragma once
#include "libs/dictionary.h"
#include "protocol.h"

#include <stdio.h>

typedef enum _request_body_type {
    RQBT_FILE,
    RQBT_STRING
} request_body_type_t;

typedef union _request_body_content {
    FILE* file;
    char* str;
} request_body_content_t;

typedef struct _request_body {
    request_body_content_t content;
    size_t length;
    size_t __ptr;
    request_body_type_t type;
} request_body_t;

// This struct contains information about a client's request
typedef struct _request {
    http_method method; 
    dictionary* headers; // a dictionary of (char*) -> (char*) 
    dictionary* params;  // a dictionary of (char*) -> (char*) 
    char* protocol;
    char* path;
    request_body_t* body;
    dictionary* form;    // a dictionary of (char*) -> (request_body_t*)
} request_t;

// Construct a request_t struct using the specified HTTP request method
request_t* request_create(http_method method);

// Destroy the passed request_t struct
void request_destroy(request_t* request);

void request_parse_query_params(request_t* request);

void request_init_str_body(request_t* request, size_t len);

void request_init_tmp_file_body(request_t* request, size_t len);

void request_convert_str_body_to_tmp_file(request_t* request);

void request_read_body(request_t* request, char* rd_buf, size_t rd_len);