#include "server.h"
#include <err.h>

response_t* test_handler(request_t* request) {
    return NULL;
}

response_t* favicon_handler(request_t* request) {
    return response_from_file(STATUS_OK, fopen("./favicon.ico", "r"));
}

int main(int argc, char** argv) {
    if (argc != 2)
        errx(EXIT_FAILURE, "./server <port>");

    server_init(argv[1]);

    server_register_route(HTTP_GET, "/v1/api/test", test_handler);
    server_register_route(HTTP_GET, "/favicon.ico", favicon_handler);

    server_launch();

    exit(0);
}
