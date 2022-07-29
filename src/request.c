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

request_t* request_create(http_method method) {
    request_t* request = malloc(sizeof(request_t));
    request->method = method;
    request->headers = string_to_string_dictionary_create();
    request->params = string_to_string_dictionary_create();
    request->url_buffer = calloc(MAX_URL_LENGTH, sizeof(char));
    request->path = NULL;
    request->body = NULL;

    return request;
}

void request_destroy(request_t* request) {
    dictionary_destroy(request->headers);
    dictionary_destroy(request->params);

    if ( request->url_buffer ) 
        free(request->url_buffer);

    if ( request->protocol ) 
        free(request->protocol);

    if ( request->path ) 
        free(request->path);

    if ( request->body )
        free(request->body);
}

const char* http_method_to_string(http_method method) {
    if ( method >= 0 && method < NUM_HTTP_METHODS ) {
        return HTTP_METHOD_STRINGS[method];
    }

    WARN("Unknown http_method enum variant: %d", method);
    return "";
}