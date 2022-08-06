#pragma once
#include "connection.h"
#include "io_utils.h"
#include "protocol.h"
#include "response.h"
#include "request.h"
#include "route.h"

// Initialize the server and bind to the specified port.
void server_init(char* port);

// Begin accepting connections.
void server_launch(void);

// Register a handler function to respond to the specified method and route.
void server_register_route(http_method http_method, char* route, route_handler_t handler);