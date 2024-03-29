#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

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

#define TIME_BUFFER_SIZE 30

#ifdef DEBUG
#define LOG(...)                                         \
    do {                                                 \
        fprintf(stderr, "DEBUG ");                       \
        fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                    \
        fprintf(stderr, "\n");                           \
    } while (0);
#else
#define LOG(...)
#endif

#ifdef DEBUG
#define WARN(...)                                        \
    do {                                                 \
        fprintf(stderr, BOLDRED"WARN ");                 \
        fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                    \
        fprintf(stderr, "\n"RESET);                      \
    } while (0);
#else
#define WARN(...)
#endif

static const char* TIME_FMT_GMT = "%a, %d %b %Y %H:%M:%S GMT";

static const char* TIME_FMT_TZ = "%a, %d %b %Y %H:%M:%S %Z";

double timespec_difftime(const struct timespec* beg, const struct timespec* end);

void format_time(char* buf, time_t time);

// buf must be a buffer of at least 30 characters
void format_current_time(char* buf);

time_t parse_time_str(const char* time_buf);

#ifndef __SKIP_LOG_REQUESTS__
void init_logging(void);

#ifdef __LOG_CONNECTS__
void print_client_connected(const char* addr, uint16_t port, int client_fd);
#endif

void print_client_request_resolution(
    const char* addr, uint16_t port, const char* method, const char* route,
    const char* protocol, int fd, int status, const char* status_str, 
    size_t request_content_len, size_t response_content_len, 
    struct timespec* connected, struct timespec* connect_finish, 
    struct timespec* send_begin);
#endif 

void print_server_details(const char* port);

void print_server_ready(void);