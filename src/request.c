#include "request.h"
#include "internals/format.h"

request_t* request_create(http_method method) {
    request_t* request = malloc(sizeof(request_t));
    request->method = method;
    request->headers = string_to_string_dictionary_create();
    request->params = string_to_string_dictionary_create();
    request->protocol = NULL;
    request->path = NULL;
    request->body = NULL;

    return request;
}

void request_destroy(request_t* request) {
    dictionary_destroy(request->headers);
    dictionary_destroy(request->params);

    if ( request->protocol ) 
        free(request->protocol);

    if ( request->path ) 
        free(request->path);

    if ( request->body )
        free(request->body);

    free(request);
}

void request_parse_url(request_t* request) {
    /// @todo maybe some url path cleaning here...
    char* question = strstr(request->path, "?");

    if (question != NULL) {
        // tructate the route to exclude params and begin parsing params
        *question++ = '\0';

        /* get the first token */
        char* token = strtok(question, "&");
        
        /* walk through other tokens */
        while( token != NULL ) {
            printf( " %s\n", token );
            
            token = strtok(NULL, s);
        }
    }
}