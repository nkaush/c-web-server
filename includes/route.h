#pragma once
#include "libs/dictionary.h"
#include "response.h"
#include "request.h"

// This handler function type will receive a request_t struct and must return 
// the response body that must be send to the client in response to the route 
// this handler serves.
typedef response_t* (*request_handler_t)(request_t*);

typedef enum _url_component_type {
    UCT_CONSTANT,
    UCT_ROUTE_PARAM,
} url_component_type_t;

typedef struct _node {
    char* component;
    dictionary* const_children;
    dictionary* var_children;
    url_component_type_t uct;
    request_handler_t* handlers;
} node_t;

node_t* node_init(char* component);

void node_destroy(node_t* node);

void register_route(http_method method, const char* route, request_handler_t handler);

request_handler_t get_handler(http_method method, const char* route);