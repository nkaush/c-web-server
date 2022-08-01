#include "server.h"
#include <err.h>

response_t* test_handler(request_t* request) {
    response_t* r = response_from_string(STATUS_OK, "{\"response\":\"hello world!\"}");
    response_set_content_type(r, CONTENT_TYPE_JSON);

    return r;
}

response_t* version(request_t* request) {
    response_t* r = response_from_string(STATUS_OK, "{\"version\":\"kqueue-server/0.0.1\"}");
    response_set_content_type(r, CONTENT_TYPE_JSON);

    return r;
}

response_t* small_file_handler(request_t* request) {
    return response_from_file(STATUS_OK, fopen("./request.txt", "r"));
}

response_t* favicon(request_t* request) {
    response_t* r = response_from_file(STATUS_OK, fopen("./favicon.png", "r"));
    response_set_content_type(r, CONTENT_TYPE_PNG);

    return r;
}

int main(int argc, char** argv) {
    if (argc != 2)
        errx(EXIT_FAILURE, "./server <port>");

    server_init(argv[1]);

    server_register_route(HTTP_GET, "/v1/api/test", test_handler);
    server_register_route(HTTP_GET, "/v1/version", version);
    server_register_route(HTTP_GET, "/request.txt", small_file_handler);
    server_register_route(HTTP_GET, "/favicon.ico", favicon);

    server_launch();

    exit(0);
}
