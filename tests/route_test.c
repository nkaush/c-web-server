#include "format.h"
#include "route.h"

response_t* root(request_t* request) { (void) request; return NULL; }

response_t* get(request_t* request) { (void) request; return NULL; }

response_t* post(request_t* request) { (void) request; return NULL; }

response_t* favicon(request_t* request) { (void) request; return NULL; }

int main(void) {
    // register_route(HTTP_GET, "/", root);
    register_route(HTTP_GET, "/favicon.ico", favicon);
    register_route(HTTP_GET, "/v1/api/test", get);
    register_route(HTTP_POST, "/v1/api/test", post);

    #define NUM_TESTS 10
    void* test_cases[NUM_TESTS][3] = {
        {(void*) HTTP_GET, "/v1/api/test", get},
        {(void*) HTTP_GET, "/v1/api/test/", get},
        {(void*) HTTP_POST, "/v1/api/test", post},
        {(void*) HTTP_PUT, "/v1/api/test", response_method_not_allowed},
        {(void*) HTTP_PUT, "/v1/api/test/", response_method_not_allowed},
        {(void*) HTTP_GET, "/", response_resource_not_found},
        {(void*) HTTP_GET, "/random", response_resource_not_found},
        {(void*) HTTP_GET, "/v1", response_resource_not_found},
        {(void*) HTTP_GET, "/v1/api", response_resource_not_found},
        {(void*) HTTP_UNKNOWN, "/random", response_malformed_request}
    };

    for (size_t i = 0; i < NUM_TESTS; ++i) {
        http_method method = (size_t) test_cases[i][0];
        char* route = test_cases[i][1];

        route_handler_t expected = test_cases[i][2];
        route_handler_t actual = find_route_handler(method, route);

        const char* mstr = http_method_to_string(method);
        printf("find_route_handler(%s, %s) ... ", mstr, route);

        if ( expected == actual ) {
            printf(BOLDGREEN"PASSED\n"RESET);
        } else {
            printf(BOLDRED"FAILED\n"RESET);
            printf("\tExpected: %s%p%s / Actual: %s%p%s\n", 
                BOLDRED, expected, RESET, BOLDRED, actual, RESET);
        }
    }
}