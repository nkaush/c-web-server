#include "types/set.h"
#include "types/bitfield.h"

#include <assert.h>
#include <string.h>

#define NUM_PRIMES 38

static size_t primes[NUM_PRIMES] = {
    17ul,         29ul,         37ul,        53ul,        67ul,
    79ul,         97ul,         131ul,       193ul,       257ul,
    389ul,        521ul,        769ul,       1031ul,      1543ul,
    2053ul,       3079ul,       6151ul,      12289ul,     24593ul,
    49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
    1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
    50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
    1610612741ul, 3221225473ul, 4294967291ul
};

typedef struct _set_node {
    struct _set_node* previous;
    struct _set_node* next;
    void* data;
} set_node_t;

struct set {
    size_t size;
    size_t capacity;
    set_node_t** nodes;
    set_node_t* head;
    set_node_t* tail;
    bitfield* should_probe;

    hash_function_type hash;
    copy_constructor_type copy_constructor;
    destructor_type destructor;
    compare comp;
};

size_t find_prime(size_t target) {
    for (size_t i = 0; i < NUM_PRIMES; ++i) {
        if (target < primes[i]) {
            return primes[i];
        }
    }

    return 0;
}

size_t set_find_key(set* this, void* elem) {
    size_t idx = this->hash(elem) % this->capacity;
    
    while ( bitfield_get(this->should_probe, idx) ) {
        // Slot is not empty, the key matches, and there is no tombstone        
        if (this->nodes[idx] != NULL 
                && !compare_equiv(this->comp, this->nodes[idx]->data, elem)) {
            return idx;
        }
            
        idx = (idx + 1) % this->capacity;
    }

    return idx;
}

void set_free_all_nodes(set* this) {
    assert(this);

    set_node_t* curr = this->head;
    set_node_t* prev = NULL;
    while ( curr != NULL ) {
        prev = curr;
        curr = curr->next;

        this->destructor(prev->data);
        free(prev);
    }
}

bool set_should_resize(set* this) {
    double a = (double) this->size;
    double b = (double) this->capacity;

    return (a / b) >= ALPHA;
}

set* set_create(hash_function_type hash_function, 
                compare comp,
                copy_constructor_type copy_constructor,
                destructor_type destructor) {
    set* this = malloc(sizeof(set));

    this->copy_constructor = copy_constructor;
    this->hash = hash_function;
    this->destructor = destructor;
    this->comp = comp;
    
    this->size = 0;
    this->head = NULL;
    this->tail = NULL;
    this->capacity = find_prime(0);
    this->should_probe = bitfield_create(this->capacity);
    this->nodes = calloc(this->capacity, sizeof(set_node_t*));
    
    return this;
}

void set_destroy(set* this) {
    assert(this);

    bitfield_destroy(this->should_probe);
    set_free_all_nodes(this);
    free(this->nodes);

    free(this);
}

set* set_union(set* s, set* t) {
    assert(s);
    assert(t);

    set* ret = set_create(
        s->hash, s->comp, s->copy_constructor, s->destructor
    );

    set_node_t* curr = s->head;
    while ( curr != NULL ) {
        set_add(ret, curr->data);
        curr = curr->next;
    }

    curr = t->head;
    while ( curr != NULL ) {
        set_add(ret, curr->data);
        curr = curr->next;
    }

    return ret;
}

set* set_intersection(set* s, set* t) {
    assert(s);
    assert(t);
    
    set* ret = set_create(
        s->hash, s->comp, s->copy_constructor, s->destructor
    );

    set* smaller_set = s->size < t->size ? s : t;
    set* larger_set = smaller_set == t ? s : t;

    set_node_t* curr = smaller_set->head;
    while ( curr != NULL ) {
        if ( set_contains(larger_set, curr->data) )
            set_add(ret, curr->data);

        curr = curr->next;
    }

    return ret;
}

set* set_difference(set* s, set* t) {
    assert(s);
    assert(t);
    
    set* ret = set_create(
        s->hash, s->comp, s->copy_constructor, s->destructor
    );

    set_node_t* curr = s->head;
    while ( curr != NULL ) {
        if ( !set_contains(t, curr->data) )
            set_add(ret, curr->data);

        curr = curr->next;
    }

    return NULL;
}

bool set_subset(set* s, set* t) {
    assert(s);
    assert(t);

    if ( t->size < s->size ) 
        return false;

    set_node_t* curr = s->head;
    while ( curr != NULL ) {
        if ( !set_contains(t, curr->data) )
            return false;
            
        curr = curr->next;
    }

    return true;
}

bool set_equals(set* s, set* t) {
    assert(s);
    assert(t);

    if ( s->size != t->size )
        return false;

    set_node_t* curr = s->head;
    while ( curr != NULL ) {
        if ( !set_contains(t, curr->data) )
            return false;
            
        curr = curr->next;
    }

    return true;
}

bool set_contains(set* this, void* element) {
    assert(this);
    return this->nodes[set_find_key(this, element)] != NULL;
}

void* set_find(set* this, void* element) {
    assert(this);
    set_node_t* node = this->nodes[set_find_key(this, element)];

    if ( node )
        return node->data;
    
    return NULL;
}

bool set_empty(set* this) {
    assert(this);
    return this->size == 0;
}

size_t set_cardinality(set* this) {
    assert(this);
    return this->size;
}

vector* set_elements(set* this) {
    assert(this);
    vector* v = shallow_vector_create();
    vector_reserve(v, this->size);

    set_node_t* curr = this->head;
    while ( curr != NULL ) {
        vector_push_back(v, curr->data);
        curr = curr->next;
    }

    return v;
}

void set_add_helper(set* this, void* element) {
    size_t index = this->hash(element) % this->capacity;
    while ( this->nodes[index] ) {
        bool cmp = compare_equiv(this->comp, this->nodes[index]->data, element);
        if ( this->nodes[index] != NULL && !cmp) 
            return; // found a repeat

        index = (index + 1) % this->capacity;
    }

    if (this->nodes[index] == NULL) { // element doesn't exist in the set
        set_node_t* node = malloc(sizeof(set_node_t));
        node->data = this->copy_constructor(element);
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

void set_add(set* this, void* element) {
    assert(this);
    
    if ( set_should_resize(this) )
        set_reserve(this, this->capacity * 2);

    set_add_helper(this, element);
}

void set_remove(set* this, void* element) {
    assert(this);
    assert(set_contains(this, element));

    size_t index = set_find_key(this, element);
    if ( this->nodes[index] != NULL ) {
        set_node_t* to_delete = this->nodes[index];
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
        
        this->destructor(to_delete->data);
        this->nodes[index] = NULL;
        free(to_delete);
        --this->size;
    }
}

void set_clear(set* this) {
    assert(this);

    set_free_all_nodes(this);
    bitfield_destroy(this->should_probe);
    this->should_probe = bitfield_create(this->capacity);

    memset(this->nodes, 0, this->capacity * sizeof(set_node_t*));
    this->head = NULL;
    this->tail = NULL;
    this->size = 0;
}

void set_reserve(set* this, size_t capacity) {
    assert(this);

    if ( capacity <= this->capacity )
        return;
    
    free(this->nodes);

    #ifdef DEBUG
    size_t old_size = this->size;
    #endif

    this->size = 0;
    bitfield_destroy(this->should_probe);
    this->capacity = find_prime(capacity);
    this->should_probe = bitfield_create(this->capacity);
    this->nodes = calloc(this->capacity, sizeof(set_node_t*));

    set_node_t* curr = this->head;
    set_node_t* prev = NULL;

    this->head = NULL;
    this->tail = NULL;

    while ( curr != NULL ) {
        set_add_helper(this, curr->data);

        this->destructor(curr->data);
        prev = curr;
        curr = curr->next;

        free(prev);
    }

    #ifdef DEBUG
    assert(this->size == old_size);
    #endif
} 

void set_shrink_to_fit(set* this) {
    assert(NULL);
    return;
}

set *string_set_create(void) {
    return set_create(
        string_hash_function, string_compare, 
        string_copy_constructor, string_destructor
    );
}

set *int_set_create(void) {
    return set_create(
        int_hash_function, int_compare, int_copy_constructor, int_destructor
    );
}

set *unsigned_int_set_create(void) {
    return set_create(
        unsigned_int_hash_function, unsigned_int_compare, 
        unsigned_int_copy_constructor, unsigned_int_destructor
    );
}

set *unsigned_long_set_create(void) {
    return set_create(
        unsigned_long_hash_function, unsigned_long_compare, 
        unsigned_long_copy_constructor, unsigned_long_destructor
    );
}
