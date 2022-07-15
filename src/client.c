#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>

#include "common.h"

int setup_client(char* host, char* port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints) );
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) { 
        fprintf(stderr, "socket: %s\n", strerror(errno));
        exit(1); 
    }

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int s = getaddrinfo(host, port, &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        freeaddrinfo(res);
        exit(1);
    }

    int ok = connect(socket_fd, res->ai_addr, res->ai_addrlen);
    if( ok == -1) { 
        fprintf(stderr, "connect: %s\n", strerror(errno)); 
        freeaddrinfo(res);
        exit(1);
    }

    freeaddrinfo(res);
    return socket_fd;
}

int main(int argc, char **argv) {
    int socket_fd = setup_client("localhost", "3000");

    FILE* in = fopen("request.txt", "r");
    struct stat s;
    fstat(fileno(in), &s);

    buffered_write_to_socket(socket_fd, in, s.st_size);
    shutdown(socket_fd, SHUT_WR);

    buffered_read_from_socket(socket_fd, stdout, 1024);
    
    return 0;
}
