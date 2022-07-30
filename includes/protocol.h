#pragma once

#define NUM_HTTP_STATUS_CODES 29
#define NUM_HTTP_METHODS 8
#define MAX_URL_LENGTH 2048

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

static const char CRLF[] = "\r\n";

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

// An incomplete enum of selected HTTP status codes
// Change the NUM_HTTP_STATUS_CODES definition in protocol.h in case any enums 
// are added or removed from this list...
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

const char* http_method_to_string(http_method method);

const char* http_status_to_string(http_status status);
