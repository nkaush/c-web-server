#include "utils/bitfield.h"
#include <assert.h>
#include <stdlib.h>

#define ALIGNMENT 8

struct bitfield {
    size_t size;
    unsigned char arr[0];
};

size_t align(size_t value, size_t alignment) {
    return ((value - 1) | (alignment - 1)) + 1;
}

bitfield* bitfield_create(size_t size) {
    assert(size > 0); 

    // round up to the nearest ALIGNMENT then divide to get array length
    size_t array_length = align(size, ALIGNMENT) / ALIGNMENT;
    bitfield* this = calloc(1, array_length + sizeof(bitfield));
    this->size = size;

    return this;
}

void bitfield_destroy(bitfield* this) {
    assert(this);
    free(this);
}

void bitfield_set_true(bitfield* this, size_t index) {
    assert(this);
    assert(index < this->size);

    this->arr[index / ALIGNMENT] |= (1 << (index % ALIGNMENT));
}

void bitfield_set_false(bitfield* this, size_t index) {
    assert(this);
    assert(index < this->size);

    this->arr[index / ALIGNMENT] &= ~(1 << (index % ALIGNMENT));
}

int bitfield_get(bitfield* this, size_t index) {
    assert(this);
    assert(index < this->size);

    return this->arr[index / ALIGNMENT] & (1 << (index % ALIGNMENT));
}

void bitfield_print(bitfield* this, FILE* f) {
    assert(this);

    fprintf(f, "[ ");
    for (size_t i = 0; i < this->size; ++i) {
        fprintf(f, "%d ", bitfield_get(this, i) ? 1 : 0);
    }
    fprintf(f, "]\n");
}
