#pragma once
#include <stdlib.h>
#include <stdio.h>

// Constants used to print in color to the command line
// Adapted from https://stackoverflow.com/a/9158263
#define RESET       "\033[0m"
#define BLACK       "\033[30m"             // Black
#define RED         "\033[31m"             // Red
#define GREEN       "\033[32m"             // Green
#define YELLOW      "\033[33m"             // Yellow
#define BLUE        "\033[34m"             // Blue
#define MAGENTA     "\033[35m"             // Magenta
#define CYAN        "\033[36m"             // Cyan
#define WHITE       "\033[37m"             // White
#define BOLDBLACK   "\033[1m\033[30m"      // Bold Black
#define BOLDRED     "\033[1m\033[31m"      // Bold Red
#define BOLDGREEN   "\033[1m\033[32m"      // Bold Green
#define BOLDYELLOW  "\033[1m\033[33m"      // Bold Yellow
#define BOLDBLUE    "\033[1m\033[34m"      // Bold Blue
#define BOLDMAGENTA "\033[1m\033[35m"      // Bold Magenta
#define BOLDCYAN    "\033[1m\033[36m"      // Bold Cyan
#define BOLDWHITE   "\033[1m\033[37m"      // Bold White

// #ifdef DEBUG
// #define LOG(...)                      \
//     do {                              \
//         fprintf(stderr, "[DEBUG] "); \
//         fprintf(stderr, __VA_ARGS__); \
//         fprintf(stderr, "\n");        \
//     } while (0);
// #else
// #define LOG(...) 
// #endif

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, "[DEBUG] "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

#define WARN(...)                      \
    do {                              \
        fprintf(stderr, BOLDRED"[WARN] "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"RESET);        \
    } while (0);

static const char* time_fmt = "%d %b %Y %H:%M:%S %Z";

void print_client_connected(char* addr, uint16_t port);

void print_server_details(char* port);

void print_server_ready(void);