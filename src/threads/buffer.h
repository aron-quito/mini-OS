/*
 * buffer.h — Issue 7.1 (SIS-60)
 *
 * Búfer circular compartido thread-safe con mutex y variables de
 * condición (not_full / not_empty).
 *
 * Toda la sincronización está encapsulada aquí; los productores y
 * consumidores solo llaman a buffer_put / buffer_get sin manipular
 * el mutex directamente.
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>

/* Capacidad del búfer: número de slots disponibles simultáneamente */
#define BUFFER_SIZE 8

typedef struct {
    int             data[BUFFER_SIZE]; /* Datos almacenados en el búfer      */
    int             head;              /* Índice de inserción (productores)   */
    int             tail;              /* Índice de extracción (consumidores) */
    int             count;            /* Número de elementos actuales         */
    pthread_mutex_t mutex;            /* Exclusión mutua sobre el búfer       */
    pthread_cond_t  not_full;         /* Señal: hay espacio para producir     */
    pthread_cond_t  not_empty;        /* Señal: hay datos para consumir       */
    int             stop_flag;        /* 1 = solicitar parada ordenada        */
} SharedBuffer;

/**
 * buffer_init — Inicializa el búfer y sus primitivas de sincronización.
 *
 * Debe llamarse UNA SOLA VEZ antes de crear los hilos.
 */
void buffer_init(SharedBuffer *b);

/**
 * buffer_destroy — Destruye el mutex y las variables de condición.
 *
 * Debe llamarse DESPUÉS de que todos los hilos hayan terminado (pthread_join).
 */
void buffer_destroy(SharedBuffer *b);

/**
 * buffer_put — Inserta un ítem en el búfer de forma segura.
 *
 * Bloquea al productor mientras el búfer está lleno (count == BUFFER_SIZE),
 * usando pthread_cond_wait en 'not_full'.
 *
 * @param b     Puntero al búfer compartido.
 * @param item  Valor entero a insertar.
 * @return 0 si el ítem fue insertado; -1 si stop_flag está activo (parada).
 */
int buffer_put(SharedBuffer *b, int item);

/**
 * buffer_get — Extrae un ítem del búfer de forma segura.
 *
 * Bloquea al consumidor mientras el búfer está vacío (count == 0),
 * usando pthread_cond_wait en 'not_empty'.
 *
 * @param b     Puntero al búfer compartido.
 * @param item  Puntero donde se escribe el valor extraído.
 * @return 0 si el ítem fue extraído; -1 si stop_flag activo y búfer vacío.
 */
int buffer_get(SharedBuffer *b, int *item);

/**
 * buffer_stop — Activa el flag de parada y desbloquea todos los hilos
 * que estén esperando en pthread_cond_wait.
 *
 * Llama a pthread_cond_broadcast en ambas variables de condición para
 * que los hilos bloqueados verifiquen stop_flag y puedan terminar limpiamente.
 *
 * @param b  Puntero al búfer compartido.
 */
void buffer_stop(SharedBuffer *b);

#endif /* BUFFER_H */
