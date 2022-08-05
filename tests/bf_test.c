#include "libs/bitfield.h"
#include <stdlib.h>
#include <time.h>

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
