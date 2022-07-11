#include "format.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <err.h>

#include "common.h"

void print_client_connected(char* addr, uint16_t port) {
    char time_buf[30];
    time_t now = time(NULL);
    strftime(time_buf, sizeof(time_buf), time_fmt, localtime(&now));

    printf("[%s] %s:%d connected...\n", time_buf, addr, port);
}

void print_server_details(char* port) {
    char host_buffer[256];
    
    int hostname = gethostname(host_buffer, sizeof(host_buffer));
    if ( hostname == -1 )
        err(EXIT_FAILURE, "gethostname");
  
    struct hostent* host_entry = gethostbyname(host_buffer);
    if ( !host_entry )
        err(EXIT_FAILURE, "gethostbyname");
  
    char* ip_buffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
    if ( !ip_buffer )
        err(EXIT_FAILURE, "inet_ntoa");
  
    printf("Hostname:\t"BOLDGREEN"%s"RESET"\nHost IP:\t"BOLDGREEN"%s:%s"RESET"\n", 
        host_buffer, ip_buffer, port
    );
}
