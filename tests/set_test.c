#include "utils/set.h"

#include <assert.h>
#include <stdio.h>

static char* program_name;

#ifdef __APPLE__
void check_leaks(void) {
    char cmd[100];
    sprintf(cmd, "export MallocStackLogging=1 && leaks %s", program_name + 2);
    printf("%s\n", cmd);
    system(cmd);
}
#endif

int main(int argc, char** argv) {
    program_name = argv[0];

    #ifdef __APPLE__
    atexit(check_leaks);
    #endif

    set* s = int_set_create();

    int total = 200;
    for (int i = 0; i < total; ++i) {
        set_add(s, &i);
        set_add(s, &i);
    }

    vector* v = set_elements(s);
    assert(vector_size(v) == (size_t) total);
    assert(set_cardinality(s) == (size_t) total);
    for (size_t i = 0; i < vector_size(v); ++i) {
        assert(((int) i) == *((int*) vector_get(v, i)));
    }
    printf("\n");
    vector_destroy(v);

    set_destroy(s);

    s = int_set_create();
    for (int i = 0; i < total; ++i)
        set_add(s, &i);

    for (int i = 0; i < total; ++i)
        set_remove(s, &i);

    assert(set_cardinality(s) == 0);
    set_destroy(s);
}
