#include <stdio.h>
#include "buffer.h"

void buffer_init(circular_buffer_t *b) {
    b->head = 0;
    b->tail = 0;
    b->count = 0;
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->not_full, NULL);
    pthread_cond_init(&b->not_empty, NULL);
}

void buffer_put(circular_buffer_t *b, int item) {
    pthread_mutex_lock(&b->mutex);
    while (b->count == BUFFER_CAPACITY) {
        pthread_cond_wait(&b->not_full, &b->mutex);
    }
    b->data[b->tail] = item;
    b->tail = (b->tail + 1) % BUFFER_CAPACITY;
    b->count++;
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
}

int buffer_get(circular_buffer_t *b) {
    pthread_mutex_lock(&b->mutex);
    while (b->count == 0) {
        pthread_cond_wait(&b->not_empty, &b->mutex);
    }
    int item = b->data[b->head];
    b->head = (b->head + 1) % BUFFER_CAPACITY;
    b->count--;
    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->mutex);
    return item;
}

void buffer_destroy(circular_buffer_t *b) {
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->not_full);
    pthread_cond_destroy(&b->not_empty);
}
