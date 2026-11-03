/*
 * buffer.c — Issue 7.1 (SIS-60)
 *
 * Implementación del búfer circular compartido thread-safe.
 *
 * Garantías de sincronización:
 *   - Todo acceso a 'data', 'head', 'tail' y 'count' está protegido
 *     por el mutex.
 *   - pthread_cond_wait previene busy-waiting en ambas condiciones.
 *   - stop_flag permite parada ordenada sin deadlock: buffer_stop hace
 *     broadcast en ambas condiciones para desbloquear todos los hilos.
 *   - buffer_put y buffer_get devuelven -1 cuando la parada fue solicitada,
 *     para que los hilos puedan salir del bucle de trabajo limpiamente.
 */

#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>

/* -----------------------------------------------------------------------
 * buffer_init
 * ----------------------------------------------------------------------- */
void buffer_init(SharedBuffer *b) {
    b->head      = 0;
    b->tail      = 0;
    b->count     = 0;
    b->stop_flag = 0;

    /* Inicializa el mutex con atributos por defecto (no recursivo) */
    if (pthread_mutex_init(&b->mutex, NULL) != 0) {
        perror("[buffer_init] pthread_mutex_init");
        exit(EXIT_FAILURE);
    }

    /* not_full: productores esperan aquí cuando el búfer está lleno */
    if (pthread_cond_init(&b->not_full, NULL) != 0) {
        perror("[buffer_init] pthread_cond_init not_full");
        pthread_mutex_destroy(&b->mutex);
        exit(EXIT_FAILURE);
    }

    /* not_empty: consumidores esperan aquí cuando el búfer está vacío */
    if (pthread_cond_init(&b->not_empty, NULL) != 0) {
        perror("[buffer_init] pthread_cond_init not_empty");
        pthread_cond_destroy(&b->not_full);
        pthread_mutex_destroy(&b->mutex);
        exit(EXIT_FAILURE);
    }
}

/* -----------------------------------------------------------------------
 * buffer_destroy
 * ----------------------------------------------------------------------- */
void buffer_destroy(SharedBuffer *b) {
    pthread_cond_destroy(&b->not_empty);
    pthread_cond_destroy(&b->not_full);
    pthread_mutex_destroy(&b->mutex);
}

/* -----------------------------------------------------------------------
 * buffer_put — Productor inserta un ítem
 * ----------------------------------------------------------------------- */
int buffer_put(SharedBuffer *b, int item) {
    pthread_mutex_lock(&b->mutex);

    /*
     * Esperar mientras el búfer está lleno Y no se ha solicitado parada.
     * Uso de while (no if) para protegerse de spurious wakeups.
     */
    while (b->count == BUFFER_SIZE && !b->stop_flag) {
        pthread_cond_wait(&b->not_full, &b->mutex);
    }

    /* Si se ordenó parada, el productor sale sin insertar */
    if (b->stop_flag) {
        pthread_mutex_unlock(&b->mutex);
        return -1;
    }

    /* Insertar el ítem en la posición 'head' y avanzar circularmente */
    b->data[b->head] = item;
    b->head = (b->head + 1) % BUFFER_SIZE;
    b->count++;

    /*
     * Señalar a los consumidores que hay al menos un ítem disponible.
     * Se usa signal (no broadcast) porque solo necesita despertar UN consumidor.
     */
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
    return 0;
}

/* -----------------------------------------------------------------------
 * buffer_get — Consumidor extrae un ítem
 * ----------------------------------------------------------------------- */
int buffer_get(SharedBuffer *b, int *item) {
    pthread_mutex_lock(&b->mutex);

    /*
     * Esperar mientras el búfer está vacío Y no se ha solicitado parada.
     * Uso de while para protegerse de spurious wakeups.
     */
    while (b->count == 0 && !b->stop_flag) {
        pthread_cond_wait(&b->not_empty, &b->mutex);
    }

    /*
     * Si se ordenó parada Y el búfer ya está vacío, el consumidor sale.
     * Si hay ítems pendientes (count > 0) aunque stop_flag == 1, los
     * consume igualmente para garantizar 0 pérdidas de datos.
     */
    if (b->count == 0 && b->stop_flag) {
        pthread_mutex_unlock(&b->mutex);
        return -1;
    }

    /* Extraer el ítem de la posición 'tail' y avanzar circularmente */
    *item  = b->data[b->tail];
    b->tail = (b->tail + 1) % BUFFER_SIZE;
    b->count--;

    /*
     * Señalar a los productores que hay espacio disponible.
     * Se usa signal (no broadcast) porque solo necesita despertar UN productor.
     */
    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->mutex);
    return 0;
}

/* -----------------------------------------------------------------------
 * buffer_stop — Parada ordenada: desbloquea todos los hilos en espera
 * ----------------------------------------------------------------------- */
void buffer_stop(SharedBuffer *b) {
    pthread_mutex_lock(&b->mutex);
    b->stop_flag = 1;

    /*
     * Broadcast (no signal) para despertar a TODOS los hilos bloqueados
     * en ambas variables de condición. Sin esto, un hilo en wait quedaría
     * bloqueado indefinidamente (deadlock al hacer pthread_join).
     */
    pthread_cond_broadcast(&b->not_full);
    pthread_cond_broadcast(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
}
