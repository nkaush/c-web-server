#pragma once
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <stdio.h>

#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)

#ifdef DEBUG
#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);
#else
#define LOG(...) 
#endif

typedef enum { 
    V_GET, 
    V_PUT, 
    V_DELETE, 
    V_LIST, 
    V_UNKNOWN 
} verb;

// This enum indicates the state of a connection.
typedef enum _connection_state {
    CS_CLIENT_CONNECTED,
    CS_HEADER_PARSED,
    CS_REQUEST_RECEIVED,
    CS_WRITING_RESPONSE
} connection_state;

/**
 * Attempts to read all count bytes from socket into buffer.
 * Assumes buffer is large enough.
 *
 * Returns the number of bytes read, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t read_all_from_socket(int socket, char *buffer, size_t count);

/**
 * Attempts to write all count bytes from buffer to socket.
 * Assumes buffer contains at least count bytes.
 *
 * Returns the number of bytes written, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t write_all_to_socket(int socket, const char *buffer, size_t count);

/**
 * @brief Read bytes from the socket into a buffer and write the buffer to the 
 * specified stream in chunks of, at most, the size of the buffer. 
 * 
 * @param socket_fd the socket to read from
 * @param out the stream to write to
 * @param count the number of bytes to read from the socket
 * @return ssize_t the number of bytes read, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t buffered_read_from_socket(int socket_fd, FILE* out, size_t count);

/**
 * @brief Read bytes from the provided stream into a buffer and write those 
 * bytes to the specified socket in chunks of, at most, the size of the buffer. 
 * 
 * @param socket_fd the socket to write to
 * @param in the stream to read from
 * @param count the number of bytes to write to the socket
 * @return ssize_t the number of bytes written, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t buffered_write_to_socket(int socket_fd, FILE* in, size_t count);

/**
 * @brief Read bytes from a socket until a newline is reached.
 * 
 * @param socket the socket to read from
 * @return char* the characters read from the socket (excluding the newline) 
 * allocated to a string on the heap.
 */
char* robust_getline(int socket);

/**
 * @brief Check if a socket has bytes to read.
 * 
 * @param fd the socket to check
 * @return the number of bytes available.
 */
int num_bytes_in_socket(int fd);
