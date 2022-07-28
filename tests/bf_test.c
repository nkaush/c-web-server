#include "types/bitfield.h"
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv) {
    // assert(argc > 1);
    // size_t size = atoi(argv[1]);
    size_t size = 20;
    bitfield* bf = bitfield_create(size);

    srand(time(NULL));

    for (int i = 0; i < 10; ++i) {
        int idx = rand() % size;
        printf("setting %d to true\n", idx);
        bitfield_set_true(bf, idx);
    }
        
    bitfield_print(bf, stdout);

    for (size_t i = 0; i < size; ++i) 
        bitfield_set_true(bf, (int) i);

    bitfield_print(bf, stdout);

    for (int i = 0; i < 10; ++i) {
        int idx = rand() % size;
        printf("setting %d to false\n", idx);
        bitfield_set_false(bf, idx);
    }

    bitfield_print(bf, stdout);

    bitfield_destroy(bf);
}
