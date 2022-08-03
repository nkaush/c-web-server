#include "internals/io_utils.h"
#include "internals/format.h"
#include "libs/vector.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <err.h>

#define BUFFER_SIZE 1024

ssize_t buffered_read_from_socket(int socket_fd, FILE* out, size_t count) {
    char* buffer = malloc(BUFFER_SIZE);
    size_t total_bytes_read = 0;
    ssize_t bytes_read = 0;
    size_t to_read = 0;

    while (total_bytes_read != count) {
        to_read = MIN(BUFFER_SIZE, count - total_bytes_read);
        bytes_read = read_all_from_socket(socket_fd, buffer, to_read);

        if (bytes_read == -1) { free(buffer); return bytes_read; }
        else if (bytes_read == 0) { free(buffer); return 0; }

        fwrite(buffer, bytes_read, sizeof(char), out);
        total_bytes_read += (size_t) bytes_read;

        if (bytes_read < (ssize_t) to_read) { break; }
    }

    free(buffer); 
    return total_bytes_read;
}

ssize_t buffered_write_to_socket(int socket_fd, FILE* in, size_t count) {
    char* buffer = malloc(BUFFER_SIZE);
    size_t total_bytes_written = 0;
    ssize_t bytes_written = 0;
    size_t to_write = 0;

    while (total_bytes_written != count) {
        to_write = MIN(BUFFER_SIZE, count - total_bytes_written);
        fread(buffer, to_write, sizeof(char), in);

        bytes_written = write_all_to_socket(socket_fd, buffer, to_write);
        if (bytes_written == -1) { free(buffer); return total_bytes_written; }
        else if (bytes_written == 0) { free(buffer); return 0; }

        total_bytes_written += (size_t) bytes_written;

        if (bytes_written < (ssize_t) to_write) { break; }
    }

    free(buffer);
    return total_bytes_written;
}

ssize_t write_all_to_socket(int socket_fd, const char *buffer, size_t count) {
    ssize_t return_code = 0;
    ssize_t bytes_sent = 0;

    size_t original = count;
    
    while (count > 0) {
        return_code = write(socket_fd, buffer + bytes_sent, count);

        if (return_code == 0) {
            LOG("Expected: %zu, Actual: %zd", original, bytes_sent);
            return bytes_sent;
        } else if (return_code > 0) {
            bytes_sent += return_code;
            count -= return_code;
        } else if (return_code == -1 && errno == EINTR) {
            continue;
        } else if (return_code == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            LOG("Expected: %zu, Actual: %zd", original, bytes_sent);
            LOG(BOLDRED"EAGAIN"RESET);
            return bytes_sent;
        } else {
            LOG("Expected: %zu, Actual: %zd", original, bytes_sent);
            return -1;
        }
    }

    LOG("Expected: %zu, Actual: %zd", original, bytes_sent);
    return bytes_sent;
}

ssize_t read_all_from_socket(int socket_fd, char *buffer, size_t count) {
    ssize_t return_code = 0;
    size_t bytes_read = 0;

    while ( count > 0 ) {
        return_code = read(socket_fd, buffer + bytes_read, count);

        if (return_code == 0) {
            return bytes_read;
        } else if (return_code > 0) {
            bytes_read += return_code;
            count -= return_code;
        } else if (return_code == -1 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }

    return bytes_read;
}

char* robust_getline(int socket_fd) {
    vector* vec = char_vector_create();
    char in[1] = { 0 };
    char delim = '\n';

    while ( *in != delim ) {
        ssize_t ret = read_all_from_socket(socket_fd, in, 1);

        if (ret < 0) {
            vector_destroy(vec);
            return NULL;
        } else if (ret == 0) {
            vector_push_back(vec, &delim);
            break;
        }

        vector_push_back(vec, in);
    }

    size_t size = vector_size(vec);
    if (size > 0) {
        char last = *((char*) vector_get(vec, size - 1));

        if (last == '\n')
            vector_pop_back(vec); // remove the last character (the newline)
    }

    size = vector_size(vec);
    char* buffer = calloc(size + 1, sizeof(char));
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = *((char*) vector_get(vec, i));
    }

    vector_destroy(vec);
    return buffer;
}

void make_socket_non_blocking(int fd) {
    // adapted from: https://github.com/eliben/code-for-blog/blob/master/2017/async-socket-server/types.c
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        err(EXIT_FAILURE, "fcntl F_GETFL");
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) 
        err(EXIT_FAILURE, "fcntl F_SETFL O_NONBLOCK");
}

int num_bytes_in_rd_socket(int fd) {
    int count;
    ioctl(fd, FIONREAD, &count);

    return count;
}

int num_bytes_in_wr_socket(int fd) {
    int count = 0;
#if defined(__linux__)
    if ( ioctl(fd, TIOCOUTQ, &count) == -1) 
        perror("ioctl TIOCOUTQ");
#elif defined(__APPLE__)
    unsigned int m = sizeof(count);
    getsockopt(fd, SOL_SOCKET, SO_NWRITE, &count, &m);
#endif
    return count;
}

int free_bytes_in_wr_socket(int fd) {
    // adapted from https://stackoverflow.com/a/36771727
    int queue_size = 0;
    unsigned int m = sizeof(queue_size);
    if ( getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*) &queue_size, &m) == -1 )
        perror("getsockopt SO_SNDBUF");

    return queue_size - num_bytes_in_wr_socket(fd);
}