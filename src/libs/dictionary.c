#include "libs/dictionary.h"
#include "libs/bitfield.h"
#include <assert.h>

#define NUM_PRIMES 38
#define ALPHA 0.7

static size_t PRIMES[NUM_PRIMES] = {
    17ul,         29ul,         37ul,        53ul,        67ul,
    79ul,         97ul,         131ul,       193ul,       257ul,
    389ul,        521ul,        769ul,       1031ul,      1543ul,
    2053ul,       3079ul,       6151ul,      12289ul,     24593ul,
    49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
    1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
    50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
    1610612741ul, 3221225473ul, 4294967291ul
};

typedef struct _dict_node {
    struct _dict_node* previous;
    struct _dict_node* next;
    void* key;
    void* value;
} dict_node_t;

struct dictionary {
    size_t size;
    size_t capacity;
    dict_node_t** nodes;
    dict_node_t* head;
    dict_node_t* tail;
    bitfield* should_probe;

    copy_constructor_type value_copy_constructor;
    copy_constructor_type key_copy_constructor;

    destructor_type value_destructor;
    destructor_type key_destructor;

    hash_function_type hash_function;
    compare comp;
};

size_t __find_next_prime(size_t target) {
    for (size_t i = 0; i < NUM_PRIMES; ++i) {
        if (target < PRIMES[i]) {
            return PRIMES[i];
        }
    }

    return 0;
}

size_t __dictionary_find_key(dictionary* this, void* elem) {
    size_t idx = this->hash_function(elem) % this->capacity;
    
    while ( bitfield_get(this->should_probe, idx) ) {
        // Slot is not empty, the key matches, and there is no tombstone        
        if (this->nodes[idx] != NULL 
                && !compare_equiv(this->comp, this->nodes[idx]->key, elem)) {
            return idx;
        }
            
        idx = (idx + 1) % this->capacity;
    }

    return idx;
}

void __dictionary_free_all_nodes(dictionary* this) {
    dict_node_t* curr = this->head;
    dict_node_t* prev = NULL;
    while ( curr != NULL ) {
        prev = curr;
        curr = curr->next;

        this->value_destructor(prev->value);
        this->key_destructor(prev->key);
        free(prev);
    }
}

static inline bool __dictionary_should_resize(dictionary* this) {
    double a = (double) this->size;
    double b = (double) this->capacity;

    return (a / b) >= ALPHA;
}

dictionary* dictionary_create_with_capacity(
        size_t capacity, hash_function_type hash_function, compare comp,
        copy_constructor_type key_copy_constructor, 
        destructor_type key_destructor,
        copy_constructor_type value_copy_constructor, 
        destructor_type value_destructor) {
    dictionary* this = malloc(sizeof(dictionary));

    this->size = 0;
    this->head = NULL;
    this->tail = NULL;
    this->capacity = capacity;
    this->should_probe = bitfield_create(this->capacity);
    this->nodes = calloc(this->capacity, sizeof(dict_node_t*));

    this->value_copy_constructor = value_copy_constructor;
    this->key_copy_constructor = key_copy_constructor;
    
    this->value_destructor = value_destructor;
    this->key_destructor = key_destructor;

    this->hash_function = hash_function;
    this->comp = comp;

    return this;
}

dictionary* dictionary_create(hash_function_type hash_function, compare comp,
                              copy_constructor_type key_copy_constructor,
                              destructor_type key_destructor,
                              copy_constructor_type value_copy_constructor,
                              destructor_type value_destructor) {
    return dictionary_create_with_capacity(
        __find_next_prime(0), hash_function, comp, key_copy_constructor,
        key_destructor, value_copy_constructor, value_destructor
    );
}

void dictionary_destroy(dictionary* this) {
    assert(this);

    bitfield_destroy(this->should_probe);
    __dictionary_free_all_nodes(this);
    free(this->nodes);

    free(this);
}

// Capacity:

bool dictionary_empty(dictionary* this) {
    assert(this);

    return this->size == 0;
}

size_t dictionary_size(dictionary* this) {
    assert(this);

    return this->size;
}

// Accessors and Modifiers

bool dictionary_contains(dictionary* this, void* key) {
    assert(this);
    return this->nodes[__dictionary_find_key(this, key)] != NULL;
}

void* dictionary_get(dictionary* this, void* key) {
    assert(dictionary_contains(this, key));
    return this->nodes[__dictionary_find_key(this, key)]->value;
}

void __dictionary_set_helper(dictionary* this, void* key, void* value) {
    size_t index = this->hash_function(key) % this->capacity;
    while ( this->nodes[index] ) {
        bool cmp = compare_equiv(this->comp, this->nodes[index]->key, key);
        if ( this->nodes[index] != NULL && !cmp) { // found a repeat
            this->value_destructor(this->nodes[index]->value);
            this->nodes[index]->value = this->value_copy_constructor(value);
            return; 
        }

        index = (index + 1) % this->capacity;
    }

    if (this->nodes[index] == NULL) { // element doesn't exist in the dict
        dict_node_t* node = malloc(sizeof(dict_node_t));
        node->key = this->key_copy_constructor(key);
        node->value = this->value_copy_constructor(value);
        node->next = NULL;

        if (this->head == NULL) {
            this->head = node;
            node->previous = NULL;
        } else {
            node->previous = this->tail;
            this->tail->next = node;
        }

        ++this->size;
        this->tail = node;
        this->nodes[index] = node;
        bitfield_set_true(this->should_probe, index);
    } // else the element already exists
}

void dictionary_reserve(dictionary* this, size_t capacity) {
    assert(this);

    if ( capacity <= this->capacity )
        return;
    
    free(this->nodes);

    #ifdef DEBUG
    size_t old_size = this->size;
    #endif

    this->size = 0;
    bitfield_destroy(this->should_probe);
    this->capacity = __find_next_prime(capacity);
    this->should_probe = bitfield_create(this->capacity);
    this->nodes = calloc(this->capacity, sizeof(dict_node_t*));

    dict_node_t* curr = this->head;
    dict_node_t* prev = NULL;

    this->head = NULL;
    this->tail = NULL;

    while ( curr != NULL ) {
        __dictionary_set_helper(this, curr->key, curr->value);

        this->value_destructor(curr->value);
        this->key_destructor(curr->key);
        prev = curr;
        curr = curr->next;

        free(prev);
    }

    #ifdef DEBUG
    assert(this->size == old_size);
    #endif
} 

void dictionary_set(dictionary* this, void* key, void* value) {
    assert(this);
    
    if ( __dictionary_should_resize(this) )
        dictionary_reserve(this, this->capacity * 2);

    __dictionary_set_helper(this, key, value);
}

void dictionary_remove(dictionary* this, void* key) {
    assert(this);
    assert(dictionary_contains(this, key));

    size_t index = __dictionary_find_key(this, key);
    if ( this->nodes[index] != NULL ) {
        dict_node_t* to_delete = this->nodes[index];
        if ( to_delete->previous ) {
            // if 'to_delete' is not the head of the Linked List
            to_delete->previous->next = to_delete->next;
        } else {
            // else make the head point to the next element
            this->head = to_delete->next;
        }

        if ( to_delete->next ) { 
            // if 'to_delete' is not the tail
            to_delete->next->previous = to_delete->previous;
        } else {
            // adjust the tail pointer
            this->tail = to_delete->previous;
        }
        
        this->value_destructor(to_delete->value);
        this->key_destructor(to_delete->key);
        this->nodes[index] = NULL;
        free(to_delete);
        --this->size;
    }
}

void dictionary_clear(dictionary* this) {
    assert(this);

    __dictionary_free_all_nodes(this);
    bitfield_destroy(this->should_probe);
    this->should_probe = bitfield_create(this->capacity);

    memset(this->nodes, 0, this->capacity * sizeof(dict_node_t*));
    this->head = NULL;
    this->tail = NULL;
    this->size = 0;
}

vector* dictionary_keys(dictionary* this) {
    assert(this);
    vector* v = shallow_vector_create();
    vector_reserve(v, this->size);

    dict_node_t* curr = this->head;
    while ( curr != NULL ) {
        vector_push_back(v, curr->key);
        curr = curr->next;
    }

    return v;
}

vector* dictionary_values(dictionary* this) {
    assert(this);
    vector* v = shallow_vector_create();
    vector_reserve(v, this->size);

    dict_node_t* curr = this->head;
    while ( curr != NULL ) {
        vector_push_back(v, curr->value);
        curr = curr->next;
    }

    return v;
}

// The following is code generated:

/*  Note: 'shallow' implies that the dictionary uses the
 *  pointer hash function and shallow comparator (for shallow keys) and
 *  shallow copy constructors and destructors.
 */

dictionary* shallow_to_shallow_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* shallow_to_string_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* shallow_to_char_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* shallow_to_double_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* shallow_to_float_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* shallow_to_int_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* shallow_to_long_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* shallow_to_short_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* shallow_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* shallow_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* shallow_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* shallow_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        shallow_hash_function, shallow_compare,
        shallow_copy_constructor, shallow_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* string_to_shallow_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* string_to_string_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* string_to_char_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* string_to_double_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* string_to_float_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* string_to_int_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* string_to_long_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* string_to_short_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* string_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* string_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* string_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* string_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        string_hash_function, string_compare,
        string_copy_constructor, string_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* char_to_shallow_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* char_to_string_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* char_to_char_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* char_to_double_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* char_to_float_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* char_to_int_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* char_to_long_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* char_to_short_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* char_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* char_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* char_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* char_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        char_hash_function, char_compare,
        char_copy_constructor, char_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* double_to_shallow_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* double_to_string_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* double_to_char_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* double_to_double_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* double_to_float_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* double_to_int_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* double_to_long_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* double_to_short_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* double_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* double_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* double_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* double_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        double_hash_function, double_compare,
        double_copy_constructor, double_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* float_to_shallow_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* float_to_string_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* float_to_char_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* float_to_double_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* float_to_float_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* float_to_int_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* float_to_long_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* float_to_short_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* float_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* float_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* float_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* float_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        float_hash_function, float_compare,
        float_copy_constructor, float_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* int_to_shallow_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* int_to_string_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* int_to_char_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* int_to_double_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* int_to_float_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* int_to_int_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* int_to_long_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* int_to_short_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* int_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* int_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* int_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* int_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        int_hash_function, int_compare,
        int_copy_constructor, int_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* long_to_shallow_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* long_to_string_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* long_to_char_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* long_to_double_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* long_to_float_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* long_to_int_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* long_to_long_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* long_to_short_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* long_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* long_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* long_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* long_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        long_hash_function, long_compare,
        long_copy_constructor, long_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* short_to_shallow_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* short_to_string_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* short_to_char_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* short_to_double_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* short_to_float_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* short_to_int_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* short_to_long_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* short_to_short_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* short_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* short_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* short_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* short_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        short_hash_function, short_compare,
        short_copy_constructor, short_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* unsigned_char_to_shallow_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* unsigned_char_to_string_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* unsigned_char_to_char_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* unsigned_char_to_double_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* unsigned_char_to_float_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* unsigned_char_to_int_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* unsigned_char_to_long_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* unsigned_char_to_short_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* unsigned_char_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* unsigned_char_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* unsigned_char_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* unsigned_char_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        unsigned_char_hash_function, unsigned_char_compare,
        unsigned_char_copy_constructor, unsigned_char_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* unsigned_int_to_shallow_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* unsigned_int_to_string_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* unsigned_int_to_char_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* unsigned_int_to_double_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* unsigned_int_to_float_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* unsigned_int_to_int_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* unsigned_int_to_long_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* unsigned_int_to_short_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* unsigned_int_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* unsigned_int_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* unsigned_int_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* unsigned_int_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        unsigned_int_hash_function, unsigned_int_compare,
        unsigned_int_copy_constructor, unsigned_int_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* unsigned_long_to_shallow_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* unsigned_long_to_string_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* unsigned_long_to_char_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* unsigned_long_to_double_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* unsigned_long_to_float_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* unsigned_long_to_int_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* unsigned_long_to_long_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* unsigned_long_to_short_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* unsigned_long_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* unsigned_long_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* unsigned_long_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* unsigned_long_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        unsigned_long_hash_function, unsigned_long_compare,
        unsigned_long_copy_constructor, unsigned_long_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

dictionary* unsigned_short_to_shallow_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        shallow_copy_constructor, shallow_destructor
    );
}

dictionary* unsigned_short_to_string_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        string_copy_constructor, string_destructor
    );
}

dictionary* unsigned_short_to_char_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        char_copy_constructor, char_destructor
    );
}

dictionary* unsigned_short_to_double_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        double_copy_constructor, double_destructor
    );
}

dictionary* unsigned_short_to_float_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        float_copy_constructor, float_destructor
    );
}

dictionary* unsigned_short_to_int_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        int_copy_constructor, int_destructor
    );
}

dictionary* unsigned_short_to_long_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        long_copy_constructor, long_destructor
    );
}

dictionary* unsigned_short_to_short_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        short_copy_constructor, short_destructor
    );
}

dictionary* unsigned_short_to_unsigned_char_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        unsigned_char_copy_constructor, unsigned_char_destructor
    );
}

dictionary* unsigned_short_to_unsigned_int_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

dictionary* unsigned_short_to_unsigned_long_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}

dictionary* unsigned_short_to_unsigned_short_dictionary_create(void) {
    return dictionary_create(
        unsigned_short_hash_function, unsigned_short_compare,
        unsigned_short_copy_constructor, unsigned_short_destructor,
        unsigned_short_copy_constructor, unsigned_short_destructor
    );
}

