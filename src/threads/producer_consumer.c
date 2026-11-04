/*
 * producer_consumer.c -- Issue 7.2 (SIS-61)
 *
 * Implementacion de los hilos trabajadores producer_worker y consumer_worker,
 * y de la funcion de orquestacion thread_demo_run.
 *
 * Dependencias:
 *   - buffer.h / buffer.c (Issue 7.1): almacen circular thread-safe.
 *   - pthread: creacion de hilos, join y atomicos.
 *
 * Garantias de correctitud:
 *   - Los contadores items_produced e items_consumed usan atomic_int
 *     (stdatomic.h, C11) para que multiples hilos los incrementen
 *     sin condicion de carrera, sin necesidad de un mutex adicional.
 *   - Ninguna lectura/escritura al bufer ocurre fuera de buffer_put/get,
 *     que ya tienen el mutex propio del bufer (Issue 7.1).
 *   - Parada ordenada: cuando todos los productores terminaron, se llama
 *     buffer_stop para despertar a los consumidores bloqueados en wait.
 */

#include "producer_consumer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>   /* atomic_int, atomic_fetch_add -- C11 */
#include <time.h>        /* clock_gettime, CLOCK_MONOTONIC       */
#include <pthread.h>

/* -----------------------------------------------------------------------
 * Contadores atomicos compartidos entre todos los hilos de una sesion.
 * Se reinician al inicio de cada thread_demo_run.
 * ----------------------------------------------------------------------- */
static atomic_int g_produced = 0;  /* Total de items insertados en el bufer */
static atomic_int g_consumed = 0;  /* Total de items extraidos del bufer    */

/* -----------------------------------------------------------------------
 * Estadisticas de la ultima sesion (consultadas por thread_get_stats).
 * ----------------------------------------------------------------------- */
static thread_stats_t s_stats = {0, 0, 0, 0, 0.0};
static int            s_has_stats = 0; /* 1 si ya se ejecuto al menos una sesion */

/* -----------------------------------------------------------------------
 * producer_worker -- Hilo productor (Issue 7.2 / SIS-61)
 *
 * Genera n_items numeros enteros aleatorios y los inserta en el bufer.
 * Imprime cada insercion mostrando:
 *   [PROD #id] Producido: <item>  |  Ocupacion: <count>/<BUFFER_SIZE>
 * ----------------------------------------------------------------------- */
void *producer_worker(void *arg) {
    WorkerArgs    *a   = (WorkerArgs *)arg;
    SharedBuffer  *buf = a->buf;
    int            id  = a->id;
    int            n   = a->n_items;

    for (int i = 0; i < n; i++) {
        /* Genera un numero aleatorio entre 1 y 999 */
        int item = (rand() % 999) + 1;

        /* Inserta en el bufer; buffer_put bloquea si esta lleno */
        if (buffer_put(buf, item) == -1) {
            /* stop_flag activo: salir anticipadamente */
            break;
        }

        /* Conteo atomico: seguro desde multiples hilos */
        atomic_fetch_add(&g_produced, 1);

        /* Leer la ocupacion actual (aproximada, solo para display) */
        pthread_mutex_lock(&buf->mutex);
        int occ = buf->count;
        pthread_mutex_unlock(&buf->mutex);

        printf("  [PROD #%d] Producido: %3d  |  Ocupacion: %d/%d\n",
               id, item, occ, BUFFER_SIZE);
        fflush(stdout);
    }

    return NULL;
}

/* -----------------------------------------------------------------------
 * consumer_worker -- Hilo consumidor (Issue 7.2 / SIS-61)
 *
 * Extrae items del bufer indefinidamente hasta que buffer_get retorna -1
 * (stop_flag activo y bufer vacio).
 * Imprime cada extraccion mostrando:
 *   [CONS #id] Consumido: <item>  |  Ocupacion: <count>/<BUFFER_SIZE>
 * ----------------------------------------------------------------------- */
void *consumer_worker(void *arg) {
    WorkerArgs   *a   = (WorkerArgs *)arg;
    SharedBuffer *buf = a->buf;
    int           id  = a->id;

    while (1) {
        int item = 0;

        /* Extrae del bufer; buffer_get bloquea si esta vacio */
        if (buffer_get(buf, &item) == -1) {
            /* stop_flag activo y bufer vacio: salir */
            break;
        }

        /* Conteo atomico */
        atomic_fetch_add(&g_consumed, 1);

        /* Leer la ocupacion actual (aproximada, solo para display) */
        pthread_mutex_lock(&buf->mutex);
        int occ = buf->count;
        pthread_mutex_unlock(&buf->mutex);

        printf("  [CONS #%d] Consumido: %3d  |  Ocupacion: %d/%d\n",
               id, item, occ, BUFFER_SIZE);
        fflush(stdout);
    }

    return NULL;
}

/* -----------------------------------------------------------------------
 * thread_demo_run -- Orquestador principal (Issue 7.2 / SIS-61)
 *
 * Flujo:
 *   1. Reiniciar contadores atomicos.
 *   2. Inicializar el SharedBuffer.
 *   3. Crear arrays dinamicos de hilos y sus argumentos.
 *   4. Lanzar num_producers hilos productores con pthread_create.
 *   5. Lanzar num_consumers hilos consumidores con pthread_create.
 *   6. Esperar a que todos los PRODUCTORES terminen (pthread_join).
 *   7. Llamar buffer_stop para despertar a los consumidores bloqueados.
 *   8. Esperar a que todos los CONSUMIDORES terminen (pthread_join).
 *   9. Destruir el bufer y registrar estadisticas + tiempo.
 * ----------------------------------------------------------------------- */
int thread_demo_run(int num_producers, int num_consumers, int items_per_prod) {
    /* --- 1. Reiniciar contadores atomicos --- */
    atomic_store(&g_produced, 0);
    atomic_store(&g_consumed, 0);

    /* --- 2. Inicializar el bufer --- */
    SharedBuffer buf;
    buffer_init(&buf);

    /* --- 3. Reservar memoria para hilos y argumentos --- */
    pthread_t  *prod_threads = malloc((size_t)num_producers * sizeof(pthread_t));
    pthread_t  *cons_threads = malloc((size_t)num_consumers * sizeof(pthread_t));
    WorkerArgs *prod_args    = malloc((size_t)num_producers * sizeof(WorkerArgs));
    WorkerArgs *cons_args    = malloc((size_t)num_consumers * sizeof(WorkerArgs));

    if (!prod_threads || !cons_threads || !prod_args || !cons_args) {
        fprintf(stderr, "[thread_demo_run] Error: malloc fallo al reservar memoria para hilos.\n");
        free(prod_threads); free(cons_threads);
        free(prod_args);    free(cons_args);
        buffer_destroy(&buf);
        return -1;
    }

    /* Semilla para numeros aleatorios (usar tiempo para variedad) */
    srand((unsigned int)time(NULL));

    /* Marca de inicio para medir tiempo total */
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    printf("\n");
    printf("  [DEMO] Iniciando sesion de hilos: %d productores, %d consumidores, %d items/prod\n",
           num_producers, num_consumers, items_per_prod);
    printf("  [DEMO] Capacidad del bufer: %d slots\n\n", BUFFER_SIZE);
    fflush(stdout);

    /* --- 4. Lanzar hilos productores --- */
    for (int i = 0; i < num_producers; i++) {
        prod_args[i].buf     = &buf;
        prod_args[i].id      = i + 1;
        prod_args[i].n_items = items_per_prod;

        if (pthread_create(&prod_threads[i], NULL, producer_worker, &prod_args[i]) != 0) {
            fprintf(stderr, "[thread_demo_run] Error: no se pudo crear el productor #%d.\n", i + 1);
            /* Parar lo que ya esta corriendo y liberar */
            buffer_stop(&buf);
            for (int j = 0; j < i; j++) pthread_join(prod_threads[j], NULL);
            free(prod_threads); free(cons_threads);
            free(prod_args);    free(cons_args);
            buffer_destroy(&buf);
            return -1;
        }
    }

    /* --- 5. Lanzar hilos consumidores --- */
    for (int i = 0; i < num_consumers; i++) {
        cons_args[i].buf     = &buf;
        cons_args[i].id      = i + 1;
        cons_args[i].n_items = 0; /* los consumidores no tienen limite fijo */

        if (pthread_create(&cons_threads[i], NULL, consumer_worker, &cons_args[i]) != 0) {
            fprintf(stderr, "[thread_demo_run] Error: no se pudo crear el consumidor #%d.\n", i + 1);
            buffer_stop(&buf);
            for (int j = 0; j < num_producers; j++) pthread_join(prod_threads[j], NULL);
            for (int j = 0; j < i; j++)            pthread_join(cons_threads[j], NULL);
            free(prod_threads); free(cons_threads);
            free(prod_args);    free(cons_args);
            buffer_destroy(&buf);
            return -1;
        }
    }

    /* --- 6. Esperar a que todos los PRODUCTORES terminen --- */
    for (int i = 0; i < num_producers; i++) {
        pthread_join(prod_threads[i], NULL);
    }

    /* --- 7. Avisar a los consumidores que ya no vendra mas datos --- */
    buffer_stop(&buf);

    /* --- 8. Esperar a que todos los CONSUMIDORES terminen --- */
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(cons_threads[i], NULL);
    }

    /* Marca de fin */
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec  - t_start.tv_sec) +
                     (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    /* --- 9. Destruir el bufer y liberar memoria --- */
    buffer_destroy(&buf);
    free(prod_threads);
    free(cons_threads);
    free(prod_args);
    free(cons_args);

    /* --- Registrar estadisticas para thread_get_stats --- */
    s_stats.producers_count    = num_producers;
    s_stats.consumers_count    = num_consumers;
    s_stats.items_produced     = atomic_load(&g_produced);
    s_stats.items_consumed     = atomic_load(&g_consumed);
    s_stats.execution_time_sec = elapsed;
    s_has_stats                = 1;

    printf("\n  [DEMO] Sesion finalizada. Usa 'thread-stats' para ver el resumen.\n\n");
    fflush(stdout);

    return 0;
}

/* -----------------------------------------------------------------------
 * thread_get_stats -- Devuelve estadisticas de la ultima sesion
 * ----------------------------------------------------------------------- */
const thread_stats_t *thread_get_stats(void) {
    if (!s_has_stats) return NULL;
    return &s_stats;
}
