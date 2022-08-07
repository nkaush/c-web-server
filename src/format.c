#include "format.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <err.h>

#define NS_PER_SECOND 1000000000

extern int h_errno;

typedef struct timespec timespec;

void format_time(char* buf, time_t time) {
    struct tm gmt;
    strftime(buf, TIME_BUFFER_SIZE, TIME_FMT_GMT, gmtime_r(&time, &gmt));
}

void format_current_time(char* buf) {
    format_time(buf, time(NULL));
}

time_t parse_time_str(const char* time_buf) {
    struct tm gmt;
    strptime(time_buf, TIME_FMT_TZ, &gmt);
    return mktime(&gmt);
}

#ifdef __LOG_REQUESTS__
void print_client_connected(const char* addr, uint16_t port, int client_fd) {
    char time_buf[TIME_BUFFER_SIZE] = { 0 };
    format_current_time(time_buf);
    printf("INFO [%s] [access] %s:%d connected on fd=%d...\n", time_buf, addr, port, client_fd);
}
#endif

double timespec_difftime(const timespec* start, const timespec* finish) {
    double delta = (finish->tv_sec - start->tv_sec) * NS_PER_SECOND;
    delta += (double) (finish->tv_nsec - start->tv_nsec);
    double duration = delta / NS_PER_SECOND;

    return duration;
}

double __compute_speed(double duration, size_t content_len) {
    double speed = (content_len * 8) / duration;
    return speed / 1000000;
}

#ifdef __LOG_REQUESTS__
void print_client_request_resolution(
        const char* addr, uint16_t port, const char* method, const char* route,
        const char* protocol, int status, const char* status_str, 
        size_t request_content_len, size_t response_content_len, 
        timespec* connected, timespec* connect_finish, timespec* send_begin) {
    timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    
    double receive_duration = timespec_difftime(connected, connect_finish);
    double process_duration = timespec_difftime(connect_finish, send_begin);
    double send_duration = timespec_difftime(send_begin, &now);
    double total_duration = timespec_difftime(connected, &now);

    double receive_speed = __compute_speed(receive_duration, request_content_len);
    double send_speed = __compute_speed(send_duration, response_content_len);

    char time_buf[TIME_BUFFER_SIZE] = { 0 };
    format_current_time(time_buf);

    char* color = BOLDRED;
    if ( status < 300 ) {
        color = BOLDGREEN;
    } else if ( status < 400 ) {
        color = BOLDMAGENTA;
    }

    printf(
        "INFO [%s] [access] %s:%d \"%s %s %s\" -- %s%d %s "RESET
        "[%zu bytes in / %zu bytes out] "
        "[%fs in / %fs handle / %fs out / %fs total] "
        "[%f Mbps in / %f Mbps out]\n", 
        time_buf, addr, port, method, route, protocol, color, status, status_str, 
        request_content_len, response_content_len, 
        receive_duration, process_duration, send_duration, total_duration, 
        receive_speed, send_speed
    );
}
#endif

void print_server_details(const char* port) {
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
