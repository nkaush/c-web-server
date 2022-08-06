#include "route.h"

response_t* root(request_t* request) { return NULL; }

response_t* get(request_t* request) { return NULL; }

response_t* post(request_t* request) { return NULL; }

int main(int argc, char** argv) {
    register_route(HTTP_GET, "/", root);
    register_route(HTTP_GET, "/v1/api/test", get);
    register_route(HTTP_POST, "/v1/api/test", post);

    request_handler_t h = get_handler(HTTP_GET, "/v1/api/test");
    printf("expected: %p vs actual %p\n", get, h);

    h = get_handler(HTTP_POST, "/v1/api/test");
    printf("expected: %p vs actual %p\n", post, h);

    h = get_handler(HTTP_POST, "/v1/api/test/");
    printf("expected: %p vs actual %p\n", post, h);

    h = get_handler(HTTP_GET, "/");
    printf("expected: %p vs actual %p\n", root, h);

    h = get_handler(HTTP_DELETE, "/v1/api/test");
    printf("expected: %p vs actual %p\n", NULL, h);

    h = get_handler(HTTP_GET, "/v2/");
    printf("expected: %p vs actual %p\n", NULL, h);

    h = get_handler(HTTP_DELETE, "/v1/api/");
    printf("expected: %p vs actual %p\n", NULL, h);
}
