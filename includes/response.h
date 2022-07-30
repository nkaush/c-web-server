#pragma once
#include "libs/dictionary.h"
#include "protocol.h"
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

// Constructs a response for 400 Bad Response with a pre-populated json message
response_t* response_bad_request(void);

// Constructs a response for 404 Not Found with a pre-populated json message
response_t* response_resource_not_found(void);

// Constructs a response for 405 Method Not Allowed with a pre-populated json message
response_t* response_method_not_allowed(void);

// Constructs a response for 414 URI Too Long with a pre-populated json message
response_t* response_uri_too_long(void);

// Response destructor. This does not need to be called by users as it will 
// automatically be called internally.
void response_destroy(response_t* response);

// Automatically adds the following headers to the response: Date, Server, Connection
// If Content-Length is not set, then sets the value to the length of the NUL-
// terminated response body string. The user must free the memory allocated for 
// the buffer passed into the function. This function returns the length of the
// response header string NOT including the NUL-byte at the end.
int response_format_header(response_t* response, char** buffer);

// Utility function to make it easier to set the Content-Type header
void response_set_content_type(response_t* response, const char* content_type);

// Utility function to make it easier to set the Content-Length header
void response_set_content_length(response_t* response, size_t length);

// Utility function to set any response header
void response_set_header(response_t* response, const char* key, const char* value);