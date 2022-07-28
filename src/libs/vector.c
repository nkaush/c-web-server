#include "libs/vector.h"
#include <assert.h>

/**
 * 'INITIAL_CAPACITY' the initial size of the dynamically.
 */
const size_t INITIAL_CAPACITY = 8;
/**
 * 'GROWTH_FACTOR' is how much the vector will grow by in automatic reallocation
 * (2 means double).
 */
const size_t GROWTH_FACTOR = 2;

struct vector {
    /* The function callback for the user to define the way they want to copy
     * elements */
    copy_constructor_type copy_constructor;

    /* The function callback for the user to define the way they want to destroy
     * elements */
    destructor_type destructor;

    /* The function callback for the user to define the way they a default
     * element to be constructed */
    default_constructor_type default_constructor;

    /* Void pointer to the beginning of an array of void pointers to arbitrary
     * data. */
    void **array;

    /**
     * The number of elements in the vector.
     * This is the number of actual objects held in the vector,
     * which is not necessarily equal to its capacity.
     */
    size_t size;

    /**
     * The size of the storage space currently allocated for the vector,
     * expressed in terms of elements.
     */
    size_t capacity;
};

/**
 * IMPLEMENTATION DETAILS
 *
 * The following is documented only in the .c file of vector,
 * since it is implementation specfic and does not concern the user:
 *
 * This vector is defined by the struct above.
 * The struct is complete as is and does not need any modifications.
 *
 * The only conditions of automatic reallocation is that
 * they should happen logarithmically compared to the growth of the size of the
 * vector inorder to achieve amortized constant time complexity for appending to
 * the vector.
 *
 * For our implementation automatic reallocation happens when -and only when-
 * adding to the vector makes its new  size surpass its current vector capacity
 * OR when the user calls on vector_reserve().
 * When this happens the new capacity will be whatever power of the
 * 'GROWTH_FACTOR' greater than or equal to the target capacity.
 * In the case when the new size exceeds the current capacity the target
 * capacity is the new size.
 * In the case when the user calls vector_reserve(n) the target capacity is 'n'
 * itself.
 */

static size_t get_new_capacity(size_t target) {
    /**
     * This function works according to 'automatic reallocation'.
     * Start at 1 and keep multiplying by the GROWTH_FACTOR until
     * you have exceeded or met your target capacity.
     */
    size_t new_capacity = 1;
    while (new_capacity < target) {
        new_capacity *= GROWTH_FACTOR;
    }
    return new_capacity;
}

vector *vector_create(copy_constructor_type copy_constructor,
                      destructor_type destructor,
                      default_constructor_type default_constructor) {
    vector* this = malloc(sizeof(vector));

    this->capacity = INITIAL_CAPACITY;
    this->size = 0;

    // allocate initial array filled with NULL pointers
    this->array = calloc(this->capacity, sizeof(void*));
    for (size_t i = 0; i < this->capacity; ++i) {
        this->array[i] = NULL;
    }

    this->default_constructor = default_constructor;
    this->copy_constructor = copy_constructor;
    this->destructor = destructor;

    return this;
}

void vector_destroy(vector *this) {
    assert(this);
    // destroy all elements within this vector
    if (this->destructor != NULL) {
        for (size_t i = 0; i < this->size; ++i) {
            this->destructor(this->array[i]);
        }
    }

    free(this->array);
    free(this);
}

void **vector_begin(vector *this) {
    return this->array + 0;
}

void **vector_end(vector *this) {
    return this->array + this->size;
}

size_t vector_size(vector *this) {
    assert(this);
    // your code here
    return this->size;
}

void vector_resize(vector *this, size_t n) {
    assert(this);

    // if we want to resize to larger than capacity, increase capacity
    if (n > this->capacity) {
        vector_reserve(this, n);
    } 
    
    if (n > this->size) {
        // add elements while we are smaller than requested
        while (this->size < n) {
            this->array[this->size++] = this->default_constructor();
        }
    } else {
        // pop elements while we are larger than expected
        while (this->size > n) {
            vector_pop_back(this);
        }
    }
}

size_t vector_capacity(vector *this) {
    assert(this);
    return this->capacity;
}

bool vector_empty(vector *this) {
    assert(this);
    return this->size == 0;
}

void vector_reserve(vector *this, size_t n) {
    assert(this);

    if (n > this->capacity) {
        this->capacity = get_new_capacity(n);
        this->array = realloc(this->array, this->capacity * sizeof(void*));
    }
}

void **vector_at(vector *this, size_t position) {
    assert(this);
    assert(position < this->size);
    return &(this->array[position]);
}

void vector_set(vector *this, size_t position, void *element) {
    assert(this);
    assert(position < this->size);

    if (this->destructor != NULL) {
        this->destructor(this->array[position]);
    }

    this->array[position] = NULL;        // this is redundant

    if (this->copy_constructor != NULL) {
        this->array[position] = this->copy_constructor(element);
    } else {
        this->array[position] = element;
    }
}

void *vector_get(vector *this, size_t position) {
    assert(this);
    assert(position < this->size);
    return this->array[position];
}

void **vector_front(vector *this) {
    assert(this);
    return &(this->array[0]);
}

void **vector_back(vector *this) {
    return &(this->array[this->size - 1]);
}

void vector_push_back(vector *this, void *element) {
    assert(this);

    // check if we need to resize
    if (this->size + 1 > this->capacity) {
        #ifdef DEBUG
        size_t old_capacity = this->capacity;
        #endif

        // add just 1 and take advantage of the growth factor multiplication code
        vector_reserve(this, get_new_capacity(this->capacity + 1));

        #ifdef DEBUG
        // ensure the capacity grew after resizing
        assert(old_capacity * GROWTH_FACTOR == this->capacity);
        #endif
    }

    // copy the element if the copy constructor is defined
    if (this->copy_constructor != NULL) {
        this->array[this->size] = this->copy_constructor(element);
    } else {
        this->array[this->size] = element;
    }
    
    ++this->size;
}

void vector_pop_back(vector *this) {
    assert(this);

    // destruct the element if the destructor is defined
    if (this->destructor != NULL) {
        this->destructor(this->array[this->size - 1]);
    }

    this->array[this->size - 1] = NULL;
    --this->size;
}

void vector_insert(vector *this, size_t position, void *element) {
    assert(this);
    assert(position <= vector_size(this));

    // check if we need to resize
    if (this->size + 1 > this->capacity) {
        #ifdef DEBUG
        size_t old_capacity = this->capacity;
        #endif

        // add just 1 and take advantage of the growth factor multiplication code
        vector_reserve(this, get_new_capacity(this->capacity + 1));

        #ifdef DEBUG
        // ensure the capacity grew after resizing
        assert(old_capacity * GROWTH_FACTOR == this->capacity);
        #endif
    }

    // copy over pointers after position 1 slot to the right
    for (size_t i = this->size; i > position; --i) 
        this->array[i] = this->array[i - 1];

    if (this->copy_constructor != NULL)
        this->array[position] = this->copy_constructor(element);
    else 
        this->array[position] = element;

    ++this->size;
}

void vector_erase(vector *this, size_t position) {
    assert(this);
    assert(position < vector_size(this));

    if (this->destructor != NULL) 
        this->destructor(this->array[position]);

    this->array[position] = NULL;

    for (size_t i = position; i < this->size - 1; ++i) 
        this->array[i] = this->array[i + 1];

    // we moved the ending element over, so set the pointer to NULL
    this->array[this->size - 1] = NULL;

    --this->size;
}

void vector_clear(vector *this) {
    for (size_t i = 0; i < this->size; ++i) {
        if (this->destructor != NULL) {
            this->destructor(this->array[i]);
        }

        this->array[i] = NULL;
    }

    this->size = 0;
}

// The following is code generated:
vector *shallow_vector_create() {
    return vector_create(shallow_copy_constructor, shallow_destructor,
                         shallow_default_constructor);
}
vector *string_vector_create() {
    return vector_create(string_copy_constructor, string_destructor,
                         string_default_constructor);
}
vector *char_vector_create() {
    return vector_create(char_copy_constructor, char_destructor,
                         char_default_constructor);
}
vector *double_vector_create() {
    return vector_create(double_copy_constructor, double_destructor,
                         double_default_constructor);
}
vector *float_vector_create() {
    return vector_create(float_copy_constructor, float_destructor,
                         float_default_constructor);
}
vector *int_vector_create() {
    return vector_create(int_copy_constructor, int_destructor,
                         int_default_constructor);
}
vector *long_vector_create() {
    return vector_create(long_copy_constructor, long_destructor,
                         long_default_constructor);
}
vector *short_vector_create() {
    return vector_create(short_copy_constructor, short_destructor,
                         short_default_constructor);
}
vector *unsigned_char_vector_create() {
    return vector_create(unsigned_char_copy_constructor,
                         unsigned_char_destructor,
                         unsigned_char_default_constructor);
}
vector *unsigned_int_vector_create() {
    return vector_create(unsigned_int_copy_constructor, unsigned_int_destructor,
                         unsigned_int_default_constructor);
}
vector *unsigned_long_vector_create() {
    return vector_create(unsigned_long_copy_constructor,
                         unsigned_long_destructor,
                         unsigned_long_default_constructor);
}
vector *unsigned_short_vector_create() {
    return vector_create(unsigned_short_copy_constructor,
                         unsigned_short_destructor,
                         unsigned_short_default_constructor);
}
