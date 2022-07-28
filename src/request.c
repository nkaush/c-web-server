#include "request.h"
#include "internals/format.h"

char* http_method_to_string(http_method method) {
    switch ( method ) {
        case HTTP_GET: 
            return "GET";
        case HTTP_HEAD:
            return "HEAD";
        case HTTP_POST:
            return "POST";
        case HTTP_PUT:
            return "PUT";
        case HTTP_DELETE:
            return "DELETE";
        case HTTP_CONNECT:
            return "CONNECT";
        case HTTP_OPTIONS:
            return "OPTIONS";
        case HTTP_TRACE:
            return "TRACE";
        default:
            WARN("Unknown http_method enum variant: %d", method);
            return "";
    }
}