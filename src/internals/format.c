#include "internals/format.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <err.h>

extern int h_errno;

void format_time(char* buf) {
    struct tm gmt;
    time_t now = time(NULL);
    strftime(buf, TIME_BUFFER_SIZE, TIME_FMT, gmtime_r(&now, &gmt));
}

void print_client_connected(char* addr, uint16_t port) {
    char time_buf[TIME_BUFFER_SIZE] = { 0 };
    format_time(time_buf);
    printf("[%s] %s:%d connected...\n", time_buf, addr, port);
}

void print_server_details(char* port) {
    char host_buffer[256];
    
    int hostname = gethostname(host_buffer, sizeof(host_buffer));
    if ( hostname == -1 ) {
        perror("gethostname");
        return;
    }
        
    struct hostent* host_entry = gethostbyname(host_buffer);
    if ( !host_entry ) {
        printf("gethostbyname: %s\n", hstrerror(h_errno));
        return;
    }
        
    char* ip_buffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
    if ( !ip_buffer )
        err(EXIT_FAILURE, "inet_ntoa");
  
    printf("Hostname:\t"BOLDGREEN"%s"RESET"\nHost IP:\t"BOLDGREEN"%s:%s"RESET"\n", 
        host_buffer, ip_buffer, port
    );
}

void print_server_ready(void) {
    printf(BOLDCYAN"Ready to accept connections..."RESET"\n");
}
