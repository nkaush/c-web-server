#include "libs/dictionary.h"

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
