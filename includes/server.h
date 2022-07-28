#pragma once
#include <sys/event.h>
#include <stdio.h>
#include <time.h>
#include "connection.h"
#include "common.h"

typedef char* (*request_handler_t)(request_t*);

void server_init(char* port);

// Begin accepting connections
void server_launch(void);