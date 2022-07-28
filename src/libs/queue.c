#include "libs/queue.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

queue_node* queue_node_init(void* data) {
    queue_node* this = malloc(sizeof(queue_node));
    this->data = data;
    this->next = NULL;

    return this;
}

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    queue* this = malloc(sizeof(queue));
    this->size = 0;
    this->max_size = max_size;
    this->head = NULL;
    this->tail = NULL;

    pthread_cond_init(&this->cv, NULL);
    pthread_mutex_init(&this->m, NULL);

    return this;
}

void queue_destroy(queue *this) {
    pthread_cond_destroy(&this->cv);
    pthread_mutex_destroy(&this->m);

    queue_node* curr = this->head;
    queue_node* old_head;

    while (curr != NULL) {
        old_head = curr;
        curr = curr->next;
        free(old_head);
    }

    free(this);
}

void queue_push(queue *this, void *data) {
    pthread_mutex_lock(&this->m);

    // block while at max size
    while (this->max_size > 0 && this->size == this->max_size) {
        pthread_cond_wait(&this->cv, &this->m);
    }

    if (this->head == NULL) {
        this->head = queue_node_init(data);
        this->tail = this->head;
    } else {
        this->tail->next = queue_node_init(data);
        this->tail = this->tail->next;
    }

    ++this->size;

    pthread_mutex_unlock(&this->m);
    pthread_cond_signal(&this->cv);
}

void *queue_pull(queue *this) {
    pthread_mutex_lock(&this->m);

    // block while at min size
    while (this->size == 0) {
        pthread_cond_wait(&this->cv, &this->m);
    }

    --this->size;
    void* ret = this->head->data;
    queue_node* old_head = this->head;
    this->head = this->head->next;
    free(old_head);

    if (this->size == 0) {
        this->tail = NULL;
    }

    pthread_mutex_unlock(&this->m);
    pthread_cond_signal(&this->cv);

    return ret;
}