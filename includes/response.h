#pragma once
#include "libs/dictionary.h"

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

typedef struct _response {
    char* body;
    http_status status;
    dictionary* headers; // a dictionary of (char*) -> (char*) 
} response_t;