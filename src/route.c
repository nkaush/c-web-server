#include "route.h"
#include "internals/format.h"

#include <err.h>

#define DEFAULT_DICT_CAPACITY 5

static node_t* root = NULL;
static const char* URL_SEP = "/";

static response_error_constructor_t RH_METHOD_NOT_ALLOWED = response_method_not_allowed;
static response_error_constructor_t RH_MALFORMED_REQUEST = response_malformed_request;
static response_error_constructor_t RH_NOT_FOUND = response_resource_not_found;

void* __node_init(void* ptr) {
    return node_init(ptr);
}

void __node_destroy(void* ptr) {
    node_destroy(ptr);
}

node_t* node_init(char* component) {
    node_t* node = malloc(sizeof(node_t));
    node->handlers = NULL;
    node->const_children = dictionary_create_with_capacity(
        DEFAULT_DICT_CAPACITY, string_hash_function, string_compare,
        string_copy_constructor, string_destructor, __node_init, __node_destroy
    );

    size_t len = strlen(component);
    if (component[0] == '<' && component[len - 1] == '>') {
        component[len - 1] = '\0';
        node->component = strdup(component + 1);
        node->uct = UCT_ROUTE_PARAM;
    } else {
        node->component = strdup(component);
        node->uct = UCT_CONSTANT;
    }

    return node;
}

void node_destroy(node_t* node) {
    if ( node->const_children ) 
        dictionary_destroy(node->const_children);

    if ( node->var_children ) 
        dictionary_destroy(node->var_children);

    if ( node->component )
        free(node->component);

    if ( node->handlers )
        free(node->handlers);

    free(node);
}

void allocate_handler_array(node_t* node) {
    node->handlers = calloc(NUM_HTTP_METHODS, sizeof(request_handler_t));
}

/// @todo investigate segfault on /
void register_route(http_method method, const char* route, request_handler_t handler) {
    if ( method == HTTP_UNKNOWN )
        errx(EXIT_FAILURE, "Cannot register handler for HTTP_UNKNOWN");
    else if ( !route ) 
        errx(EXIT_FAILURE, "Cannot register handler for NULL route");
    else if ( !handler )
        errx(EXIT_FAILURE, "Cannot register handler for NULL handler");
    
    if ( !root )
        root = node_init("");

    char* route_dup = strdup(route + 1);
    char* route_str = route_dup;
    char* token = NULL;
    node_t* curr = root;
    while ( ( token = strsep(&route_str, URL_SEP) ) ) {
        if ( !strcmp(token, "") )
            break;

        if ( !dictionary_contains(curr->const_children, token) )
            dictionary_set(curr->const_children, token, token);

        curr = dictionary_get(curr->const_children, token);
    }

    if ( !curr->handlers )
        allocate_handler_array(curr);

    if ( curr->handlers[(size_t) method] )
        WARN("Redefinition of route '%s %s'", http_method_to_string(method), route);

    curr->handlers[method] = handler;
    free(route_dup);
}

response_t* handle_route(request_t* request) {
    if ( request->method == HTTP_UNKNOWN )
        return RH_MALFORMED_REQUEST();

    char* route_dup = strdup(request->path + 1);
    char* route_str = route_dup;
    char* token = NULL;
    node_t* curr = root;
    while ( ( token = strsep(&route_str, URL_SEP) ) ) {
        if ( !strcmp(token, "") )
            break;

        /// @todo handle variable case
        if ( !dictionary_contains(curr->const_children, token) ) {
            free(route_dup);
            return NULL;
        }

        curr = dictionary_get(curr->const_children, token);
    }

    free(route_dup);
    if ( curr->handlers ) { // the route exists...
        request_handler_t handler = curr->handlers[request->method];

        if ( !handler ) // however, the route is not defined for the requested method
            return RH_METHOD_NOT_ALLOWED();

        return handler(request);
    } else { // the route does not exist...
        return RH_NOT_FOUND();
    }
}