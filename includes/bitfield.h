#pragma once
// Adapted from https://stackoverflow.com/a/30590727
#include <unistd.h>
#include <stdio.h>

// forward declare the type
typedef struct bitfield bitfield;

bitfield* bitfield_create(size_t size);

void bitfield_destroy(bitfield* this);

void bitfield_set_true(bitfield* this, size_t index);

void bitfield_set_false(bitfield* this, size_t index);

int bitfield_get(bitfield* this, size_t index);

void bitfield_print(bitfield* this, FILE* f);