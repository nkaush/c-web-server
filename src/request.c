#include "request.h"
#include "internals/format.h"

static const char* HTTP_METHOD_STRINGS[NUM_HTTP_METHODS] = {
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "CONNECT",
    "OPTIONS",
    "TRACE"
};

const char* http_method_to_string(http_method method) {
    if ( method >= 0 && method < NUM_HTTP_METHODS ) {
        return HTTP_METHOD_STRINGS[method];
    }

    WARN("Unknown http_method enum variant: %d", method);
    return "";
}