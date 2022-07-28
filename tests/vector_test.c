#include "libs/vector.h"
#include <stdio.h>

#define BOLDGREEN "\033[1m\033[32m"
#define BOLDRED   "\033[1m\033[31m"
#define RESET     "\033[0m"

int test_iterator_begin_end(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();
    int vals[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int* vals_iter = vals;

    for (size_t i = 0; i < 10; ++i) 
        vector_push_back(vec, (void*) vals_iter++);

    if ( vector_size(vec) != 10 ) 
        did_pass = 0;

    int** begin = (int**) vector_begin(vec);
    int** end = (int**) vector_end(vec);
    vals_iter = vals;

    for (int** it = begin; it != end; ++it) {
        if (**it != *vals_iter)
            did_pass = 0;

        ++vals_iter;
    }
    
    vector_destroy(vec);
    return did_pass;
}

int test_iterator_loop(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();
    int vals[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int* vals_iter = vals;

    for (size_t i = 0; i < 10; ++i) 
        vector_push_back(vec, (void*) vals_iter++);

    if ( vector_size(vec) != 10 ) 
        did_pass = 0;

    vals_iter = vals;
    for (size_t i = 0; i < vector_size(vec); ++i) {
        if (*((int*) vector_get(vec, i)) != *vals_iter)
            did_pass = 0;

        ++vals_iter;
    }

    vector_destroy(vec);
    return did_pass;
}

int test_vector_size(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();
    int vals[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int* vals_iter = vals;

    for (size_t i = 0; i < 10; ++i) {
        vector_push_back(vec, (void*) vals_iter++);

        if ( vector_size(vec) != i + 1 )
            did_pass = 0; 
    }

    for (size_t i = 0; i < 10; ++i) {
        vector_pop_back(vec);

        if ( vector_size(vec) != 9 - i)
            did_pass = 0; 
    }

    vector_destroy(vec);
    return did_pass;
}

int test_vector_capacity(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();
    int vals[17] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    int* vals_iter = vals;

    for (size_t i = 0; i < 17; ++i) {
        vector_push_back(vec, (void*) vals_iter++);

        if (i == 7 && vector_capacity(vec) != 8) 
            did_pass = 0;
        if (i == 8 && vector_capacity(vec) != 16) 
            did_pass = 0;
        if (i == 15 && vector_capacity(vec) != 16) 
            did_pass = 0;
        if (i == 16 && vector_capacity(vec) != 32) 
            did_pass = 0;
    }

    if ( vector_capacity(vec) != 32 ) 
        did_pass = 0;

    // check clearing does not change capacity
    vector_clear(vec);
    if ( vector_capacity(vec) != 32 )
        did_pass = 0;

    vector_destroy(vec);
    return did_pass;
}

int test_vector_empty(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();

    if ( !vector_empty(vec) ) 
        did_pass = 0;
    
    int i = 3;
    vector_push_back(vec, &i);

    if ( vector_empty(vec) ) 
        did_pass = 0;

    vector_push_back(vec, &i);

    if ( vector_empty(vec) ) 
        did_pass = 0;

    vector_pop_back(vec);

    if ( vector_empty(vec) ) 
        did_pass = 0;

    vector_pop_back(vec);

    if ( !vector_empty(vec) ) 
        did_pass = 0;

    vector_destroy(vec);
    return did_pass;
}

int test_vector_reserve(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();

    // test reserve smaller than current capacity does nothing
    size_t old_capacity = vector_capacity(vec);
    vector_reserve(vec, old_capacity - 1);

    if ( old_capacity != vector_capacity(vec) || 0 != vector_size(vec) )
        did_pass = 0;

    // test reserve current capacity does nothing
    old_capacity = vector_capacity(vec);
    vector_reserve(vec, old_capacity);

    if ( old_capacity != vector_capacity(vec) || 0 != vector_size(vec) )
        did_pass = 0;

    // test reserving larger than current capacity allocates more space
    old_capacity = vector_capacity(vec);
    vector_reserve(vec, old_capacity + 1);

    if ( old_capacity == vector_capacity(vec) || 0 != vector_size(vec) )
        did_pass = 0;

    vector_destroy(vec);
    return did_pass;
}

int test_vector_set(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();
    int* default_ptr = int_default_constructor();
    int default_value = *((int*) default_ptr);
    free(default_ptr);

    vector_resize(vec, 100);

    for (int i = 0; i < 200; i += 2) {
        if ( *((int*) vector_get(vec, i / 2)) != default_value )
            did_pass = 0;
        
        vector_set(vec, i / 2, (void*) &i);
    }

    for (int i = 0; i < 100; ++i) {
        if ( *((int*) vector_get(vec, i)) != 2 * i )
            did_pass = 0;
    }

    vector_destroy(vec);
    return did_pass;
}

int test_vector_push_back(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();

    // stress test with 1 million elements
    for (int i = 0; i < 1000000; ++i) {
        vector_push_back(vec, (void*) &i);
    }

    for (int i = 0; i < 1000000; ++i) {
        if (*((int*) vector_get(vec, i)) != i) 
            did_pass = 0;
    }

    vector_destroy(vec);
    return did_pass;
}

int test_vector_pop_back(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();

    for (int i = 0; i < 1000; ++i) {
        vector_push_back(vec, (void*) &i);
    }

    for (int i = 998; i >= 0; --i) {
        vector_pop_back(vec);
        if (**((int**) vector_back(vec)) != i) 
            did_pass = 0;
    }

    vector_destroy(vec);
    return did_pass;
}

int test_vector_erase(void) {
    int did_pass = 1;

    vector* vec = int_vector_create();
    vector_resize(vec, 10);

    for (int i = 0; i < 10; ++i) {
        vector_set(vec, i, (void*) &i);
    }

    if ( vector_size(vec) != 10 )
        did_pass = 0;

    vector_erase(vec, 9);

    if ( vector_size(vec) != 9 )
        did_pass = 0;

    if ( **((int**) vector_back(vec)) != 8 ) 
        did_pass = 0;

    vector_destroy(vec);
    return did_pass;
}

int test_vector_front(void) {
    int did_pass = 1;

    return did_pass;
}

int test_vector_at(void) {
    int did_pass = 1;

    return did_pass;
}

int test_vector_back(void) {
        int did_pass = 1;

    return did_pass;
}

int test_vector_clear(void) {
    int did_pass = 1;

    return did_pass;
}

int test_vector_insert(void) {
    int did_pass = 1;

    return did_pass;
}

int main(int argc, char *argv[]) {
    #define NUM_TESTS 15

    char* test_names[NUM_TESTS] = {
        "test_iterator_begin_end",
        "test_iterator_loop",
        "test_vector_size",
        "test_vector_capacity",
        "test_vector_empty",
        "test_vector_reserve",
        "test_vector_at",
        "test_vector_set",
        "test_vector_front",
        "test_vector_back",
        "test_vector_push_back",
        "test_vector_pop_back",
        "test_vector_clear",
        "test_vector_erase",
        "test_vector_insert"
    };

    int (*tests[NUM_TESTS])(void) = {
        test_iterator_begin_end,
        test_iterator_loop,
        test_vector_size,
        test_vector_capacity,
        test_vector_empty,
        test_vector_reserve,
        test_vector_at,
        test_vector_set,
        test_vector_front,
        test_vector_back,
        test_vector_push_back,
        test_vector_pop_back,
        test_vector_clear,
        test_vector_erase,
        test_vector_insert
    };

    printf("Running %d test cases from %s...\n\n", NUM_TESTS, argv[0]);

    for (int i = 0; i < NUM_TESTS; ++i) {
        printf("%s ...", test_names[i]);

        if ( tests[i]() ) {
            printf(BOLDGREEN" PASSED"RESET"\n");
        } else {
            printf(BOLDRED" FAILED"RESET"\n");
        }
    }

    printf("\n");
}
