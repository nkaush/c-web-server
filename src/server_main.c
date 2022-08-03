#include "server.h"
#include <err.h>

#ifdef __APPLE__
static char* program_name;

void check_leaks(void) {
    char cmd[100];
    sprintf(cmd, "export MallocStackLogging=1 && leaks %s", program_name + 2);
    printf("%s\n", cmd);
    system(cmd);
}  
#endif

response_t* test_handler(request_t* request) {
    response_t* r = response_from_string(STATUS_OK, "{\"response\":\"hello world!\"}");
    response_set_content_type(r, CONTENT_TYPE_JSON);

    return r;
}

response_t* favicon(request_t* request) {
    response_t* r = response_from_file(STATUS_OK, fopen("./favicon.png", "r"));
    response_set_content_type(r, CONTENT_TYPE_PNG);

    return r;
}

response_t* handout(request_t* request) {
    response_t* r = response_from_file(STATUS_OK, fopen("./handouts.pdf", "r"));
    response_set_content_type(r, CONTENT_TYPE_PDF);

    return r;
}

int main(int argc, char** argv) {
#if defined(__APPLE__) && defined(DEBUG)
    program_name = argv[0];
    atexit(check_leaks);
#endif

    if (argc != 2)
        errx(EXIT_FAILURE, "./server <port>");

    server_init(argv[1]);

    server_register_route(HTTP_GET, "/v1/api/test", test_handler);
    server_register_route(HTTP_GET, "/favicon.ico", favicon);
    server_register_route(HTTP_GET, "/handout.pdf", handout);
    
    server_launch();

    exit(0);
}
