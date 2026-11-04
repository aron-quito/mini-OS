/*
 * producer_consumer.h -- Issue 7.2 (SIS-61)
 *
 * Declara las funciones de hilo worker (producer_worker / consumer_worker)
 * y la interfaz publica de alto nivel para lanzar la demo concurrente
 * (thread_demo_run) y consultar estadisticas (thread_get_stats).
 *
 * La logica de sincronizacion esta completamente encapsulada en buffer.h/c
 * (Issue 7.1); este modulo solo orquesta los hilos y recoge metricas.
 */

#ifndef PRODUCER_CONSUMER_H
#define PRODUCER_CONSUMER_H

#include "buffer.h"   /* SharedBuffer, BUFFER_SIZE */

/* -----------------------------------------------------------------------
 * Argumentos que recibe cada hilo al crearse con pthread_create.
 *
 * Tanto producer_worker como consumer_worker reciben un puntero a esta
 * estructura casteado a (void *), lo convierten internamente y leen
 * solo los campos que necesitan.
 * ----------------------------------------------------------------------- */
typedef struct {
    SharedBuffer *buf;      /* Bufer circular compartido (Issue 7.1)        */
    int           id;       /* Numero de hilo dentro de su grupo (1, 2, ...) */
    int           n_items;  /* Items que el PRODUCTOR debe generar (ignorado
                               por los consumidores)                         */
} WorkerArgs;

/* -----------------------------------------------------------------------
 * Estadisticas de la ultima sesion de hilos.
 *
 * Llenada por thread_demo_run; consultada por thread_get_stats (Issue 7.4).
 * ----------------------------------------------------------------------- */
typedef struct {
    int    producers_count;    /* Numero de hilos productores usados     */
    int    consumers_count;    /* Numero de hilos consumidores usados    */
    int    items_produced;     /* Total de items insertados en el bufer  */
    int    items_consumed;     /* Total de items extraidos del bufer     */
    double execution_time_sec; /* Tiempo total de la sesion en segundos  */
} thread_stats_t;

/* -----------------------------------------------------------------------
 * Funciones worker -- punto de entrada de cada hilo
 *
 * Firma compatible con pthread_create: recibe (void *) y devuelve (void *).
 * ----------------------------------------------------------------------- */

/**
 * producer_worker -- Hilo productor (Issue 7.2 / SIS-61)
 *
 * Genera numeros enteros aleatorios y los inserta en el bufer compartido
 * llamando a buffer_put. Imprime en consola cada insercion con su ID,
 * el item generado y la ocupacion actual del bufer.
 *
 * Termina cuando ha producido n_items items O cuando buffer_put retorna -1
 * (stop_flag activo).
 */
void *producer_worker(void *arg);

/**
 * consumer_worker -- Hilo consumidor (Issue 7.2 / SIS-61)
 *
 * Extrae items del bufer llamando a buffer_get y los "procesa" (los imprime).
 * Imprime en consola cada extraccion con su ID, el item consumido y la
 * ocupacion actual del bufer.
 *
 * Termina cuando buffer_get retorna -1 (stop_flag activo y bufer vacio).
 */
void *consumer_worker(void *arg);

/* -----------------------------------------------------------------------
 * Interfaz de alto nivel (Issue 7.3 conecta esto al comando thread-demo)
 * ----------------------------------------------------------------------- */

/**
 * thread_demo_run -- Lanza la demo concurrente Productor-Consumidor.
 *
 * Crea num_producers hilos productores y num_consumers hilos consumidores,
 * espera a que todos terminen con pthread_join, registra estadisticas y
 * mide el tiempo total con clock_gettime(CLOCK_MONOTONIC).
 *
 * @param num_producers  Numero de hilos productores (>= 2).
 * @param num_consumers  Numero de hilos consumidores (>= 2).
 * @param items_per_prod Items que genera cada productor.
 * @return 0 en exito, -1 si falla la creacion de algun hilo.
 */
int thread_demo_run(int num_producers, int num_consumers, int items_per_prod);

/**
 * thread_get_stats -- Devuelve estadisticas de la ultima sesion.
 *
 * Retorna NULL si no se ha ejecutado ninguna sesion aun.
 */
const thread_stats_t *thread_get_stats(void);

#endif /* PRODUCER_CONSUMER_H */
