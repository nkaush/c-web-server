#include "types/dictionary.h"

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

    dictionary* d = int_to_int_dictionary_create();

    int total = 1600;
    for (int i = 0; i < total; ++i) {
        dictionary_set(d, &i, &i);

        assert(i == *((int*) dictionary_get(d, &i)));

        int x = i * 2;
        dictionary_set(d, &i, &x);
        assert(i * 2 == *((int*) dictionary_get(d, &i)));
    }

    vector* k = dictionary_keys(d);
    vector* v = dictionary_values(d);

    assert(vector_size(v) == (size_t) total);
    assert(vector_size(k) == (size_t) total);
    assert(dictionary_size(d) == (size_t) total);

    for (size_t i = 0; i < vector_size(v); ++i) {
        assert(((int) i) == *((int*) vector_get(k, i)));
        assert(((int) i) * 2 == *((int*) vector_get(v, i)));
    }
    printf("\n");
    vector_destroy(k);
    vector_destroy(v);
    dictionary_destroy(d);
}
