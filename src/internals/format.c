#include "internals/format.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <err.h>

#define NS_PER_SECOND 1000000000

extern int h_errno;

void format_time(char* buf, time_t time) {
    struct tm gmt;
    strftime(buf, TIME_BUFFER_SIZE, TIME_FMT, gmtime_r(&time, &gmt));
}

void format_current_time(char* buf) {
    format_time(buf, time(NULL));
}

void print_client_connected(char* addr, uint16_t port) {
    char time_buf[TIME_BUFFER_SIZE] = { 0 };
    format_current_time(time_buf);
    printf("INFO [%s] [access] %s:%d connected...\n", time_buf, addr, port);
}

double timespec_difftime(struct timespec* start, struct timespec* finish) {
    double delta = (finish->tv_sec - start->tv_sec) * NS_PER_SECOND;
    delta += (double) (finish->tv_nsec - start->tv_nsec);
    double duration = delta / NS_PER_SECOND;

    return duration;
}

void print_client_request_resolution(
        const char* addr, uint16_t port, const char* method, const char* route,
        const char* protocol, int status, const char* status_str, 
        size_t content_len, struct timespec* start) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    double duration = timespec_difftime(start, &now);

    char time_buf[TIME_BUFFER_SIZE] = { 0 };
    format_current_time(time_buf);

    char* color = BOLDRED;
    if ( status < 300 ) {
        color = BOLDGREEN;
    } else if ( status < 400 ) {
        color = BOLDMAGENTA;
    } 

    // (bytes / 1000000) / (Download Speed In Megabits / 8) = time
    // (bytes / 1000000) / time = (Download Speed In Megabits / 8)
    // (bytes * 8) / (1000000 * time) = speed

    double speed = (content_len * 8) / duration;
    speed /= 1000000;

    printf(
        "INFO [%s] [access] %s:%d \"%s %s %s\" -- %s%d %s "RESET
        "[%zu bytes sent] [%fs] [%f Mbps]\n", 
        time_buf, addr, port, method, route, protocol, color,
        status, status_str, content_len, duration, speed
    );
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
