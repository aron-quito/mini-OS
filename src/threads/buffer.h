#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>

#define BUFFER_CAPACITY 8

typedef struct {
    int data[BUFFER_CAPACITY];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} circular_buffer_t;

/**
 * @brief Inicializa el búfer circular y sus primitivas de sincronización.
 */
void buffer_init(circular_buffer_t *b);

/**
 * @brief Inserta un elemento de manera segura bloqueando si está lleno.
 */
void buffer_put(circular_buffer_t *b, int item);

/**
 * @brief Extrae un elemento de manera segura bloqueando si está vacío.
 */
int buffer_get(circular_buffer_t *b);

/**
 * @brief Destruye el mutex y las variables de condición del búfer.
 */
void buffer_destroy(circular_buffer_t *b);

#endif // BUFFER_H
