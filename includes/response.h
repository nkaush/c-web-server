#pragma once
#include "libs/dictionary.h"
#include <stdio.h>

// This is an incomplete list of content types to use when setting the content type header sent to the client

#define CONTENT_TYPE_CSS   "text/css"
#define CONTENT_TYPE_CSV   "text/csv"
#define CONTENT_TYPE_HTML  "text/html"
#define CONTENT_TYPE_PLAIN "text/plain"
#define CONTENT_TYPE_XML   "application/xml"
#define CONTENT_TYPE_PDF   "application/pdf"
#define CONTENT_TYPE_ZIP   "application/zip"
#define CONTENT_TYPE_JSON  "application/json"
#define CONTENT_TYPE_JS    "application/javascript"

#define NUM_HTTP_STATUS_CODES 29

// An incomplete enum of selected HTTP status codes
typedef enum _http_status {
    STATUS_OK                         = 200,
    STATUS_CREATED                    = 201,
    STATUS_ACCEPTED                   = 202,
    STATUS_NO_CONTENT                 = 204,
    STATUS_MOVED_PERMANENTLY          = 301,
    STATUS_FOUND                      = 302,
    STATUS_SEE_OTHER                  = 303,
    STATUS_NOT_MODIFIED               = 304,
    STATUS_TEMPORARY_REDIRECT         = 307,
    STATUS_PERMANENT_REDIRECT         = 308,
    STATUS_BAD_REQUEST                = 400,
    STATUS_UNAUTHORIZED               = 401,
    STATUS_FORBIDDEN                  = 403,
    STATUS_NOT_FOUND                  = 404,
    STATUS_METHOD_NOT_ALLOWED         = 405,
    STATUS_REQUEST_TIMEOUT            = 408,
    STATUS_LENGTH_REQUIRED            = 411,
    STATUS_PAYLOAD_TOO_LARGE          = 413,
    STATUS_URI_TOO_LONG               = 414,
    STATUS_UNSUPPORTED_MEDIA_TYPE     = 415,
    STATUS_IM_A_TEAPOT                = 418,
    STATUS_TOO_MANY_REQUESTS          = 429,
    STATUS_INTERNAL_SERVER_ERROR      = 500,
    STATUS_NOT_IMPLEMENTED            = 501,
    STATUS_BAD_GATEWAY                = 502,
    STATUS_SERVICE_UNAVAILABLE        = 503,
    STATUS_GATEWAY_TIMEOUT            = 504,
    STATUS_HTTP_VERSION_NOT_SUPPORTED = 505,
    STATUS_INSUFFICIENT_STORAGE       = 507
} http_status;

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

response_t* response_from_file(http_status status, FILE* file);

response_t* response_from_string(http_status status, const char* body);

response_t* response_empty(http_status status);

response_t* response_method_not_allowed(void);

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