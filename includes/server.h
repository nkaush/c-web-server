#pragma once
#include "internals/connection.h"
#include "request.h"
#include "internals/common.h"

// This handler function type will receive a request_t struct and must return 
// the response body that must be send to the client in response to the route 
// this handler serves.
typedef char* (*request_handler_t)(request_t*);

// Initialize the server and bind to the specified port.
void server_init(char* port);

// Begin accepting connections.
void server_launch(void);

// Register a handler function to respond to the specified method and route.
void server_register_route(verb http_method, char* route, request_handler_t handler);