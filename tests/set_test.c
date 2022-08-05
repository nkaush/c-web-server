#include "libs/set.h"

#include <unistd.h>
#include <assert.h>
#include <stdio.h>

#if defined(__APPLE__) && defined(DEBUG)
void check_leaks(void) {
    char cmd[100];
    sprintf(cmd, "leaks --list %d", getpid());
    printf("%s\n", cmd);
    system(cmd);
}  
#endif

int main(int argc, char** argv) {
#if defined(__APPLE__) && defined(DEBUG)
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
