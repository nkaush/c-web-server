#pragma once
#include "dictionary.h"
#include "protocol.h"
#include "request.h"
#include <stdio.h>

typedef enum _response_type {
    RT_FILE,
    RT_STRING,
    RT_EMPTY
} response_type;

typedef union _body_content {
    FILE* file;
    const char* body;
} body_content_t;

// This struct represents a response to a HTTP request. The server will format
// the fields in this structure and send the formatted response to the client
// who made the request that corresponds to this response.
// 
// The server will automatically add the following headers to the response: 
// Date, Server, Connection. If the Content-Length header is not set, then the 
// server sets the value to the length of the NUL-terminated response body 
// string. 
typedef struct _response {
    body_content_t body_content;
    dictionary* headers; // a dictionary of (char*) -> (char*) 
    http_status status;
    response_type rt;
} response_t;

/// STANDARD RESPONSE CONSTRUCTORS

// Construct a response that will send the contents of a file as the response body
response_t* response_from_file(http_status status, FILE* file);

// Construct a response that will send the contents of the passed string 
// UP UNTIL THE FIRST NULL BYTE!
response_t* response_from_string(http_status status, const char* body);

// Construct a response that will send an empty body
response_t* response_empty(http_status status);

/// ERROR RESPONSE CONSTRUCTORS

// Construct a response for 304 Not Modified
response_t* response_not_modified(request_t* request);

// Constructs a response for 400 Bad Response with a pre-populated json message
response_t* response_malformed_request(request_t* request);

// Constructs a response for 400 Bad Response with a pre-populated json message
response_t* response_bad_request(request_t* request);

// Constructs a response for 404 Not Found with a pre-populated json message
response_t* response_resource_not_found(request_t* request);

// Constructs a response for 405 Method Not Allowed with a pre-populated json message
response_t* response_method_not_allowed(request_t* request);

// Constructs a response for 411 Length Required with a pre-populated json message
response_t* response_length_required(request_t* request);

// Constructs a response for 414 URI Too Long with a pre-populated json message
response_t* response_uri_too_long(request_t* request);

// Response destructor. This does not need to be called by users as it will 
// automatically be called internally.
void response_destroy(response_t* response);

#ifndef __DISABLE_HANDLE_IF_MODIFIED_SINCE__
void response_try_optimize_if_not_modified_since(
    response_t** response, char* target_date);
#endif

// Utility function to make it easier to set the Content-Type header
void response_set_content_type(response_t* response, const char* content_type);

// Utility function to make it easier to set the Content-Length header
void response_set_content_length(response_t* response, size_t length);

// Utility function to set any response header
void response_set_header(response_t* response, const char* key, const char* value);