#include "request.h"
#include "internals/format.h"

request_t* request_create(http_method method) {
    request_t* request = malloc(sizeof(request_t));
    request->method = method;
    request->headers = string_to_string_dictionary_create();
    request->params = NULL;
    request->protocol = NULL;
    request->path = NULL;
    request->body = NULL;

    return request;
}

void request_destroy_body(request_body_t* body) {
    if (body->type == RQBT_FILE) {
        fclose(body->content.file);
    } else {
        free(body->content.str);
    }

    free(body);
}

void request_destroy(request_t* request) {
    dictionary_destroy(request->headers);

    if ( request->params )
        dictionary_destroy(request->params);

    if ( request->protocol ) 
        free(request->protocol);

    if ( request->path ) 
        free(request->path);

    if ( request->body )
        request_destroy_body(request->body);

    free(request);
}

void request_parse_query_params(request_t* request) {
    static const char* QUERY_PARAM_BEGIN = "?";
    static const char* QUERY_PARAM_DELIM = "&";
    static const char* QUERY_PARAM_SEP = "=";

    /// @todo maybe some url path cleaning here...
    char* question = strstr(request->path, QUERY_PARAM_BEGIN);

    if ( !question ) { return; }

    // tructate the route to exclude params and begin parsing params
    *question++ = '\0';
    request->params = string_to_string_dictionary_create();

    char* token = NULL;
    while ( ( token = strsep(&question, QUERY_PARAM_DELIM) ) ) {
        char* key = strsep(&token, QUERY_PARAM_SEP);
        
        // Do not add query param if the value is NULL since url is invalid
        // ex: ?hello=world& --> [("hello"="world"), (""=NULL)]
        // ex: ?hello=world&test --> [("hello"="world"), ("test"=NULL)]
        if ( token )
            dictionary_set(request->params, key, token);
    }
}

void request_init_str_body(request_t* request, size_t len) {
    request_body_t* body = malloc(sizeof(request_body_t));
    body->type = RQBT_STRING;
    body->content.str = calloc(len, sizeof(char));
    body->length = len;

    request->body = body;
}

void request_init_tmp_file_body(request_t* request, size_t len) {
    request_body_t* body = malloc(sizeof(request_body_t));
    body->type = RQBT_FILE;
    body->content.file = fopen("tmp.pdf", "a");
    body->length = len;

    request->body = body;
}
