#ifndef PRODUCER_CONSUMER_H
#define PRODUCER_CONSUMER_H

typedef struct {
    int producers_count;
    int consumers_count;
    int items_produced;
    int items_consumed;
    double execution_time_sec;
} thread_stats_t;

/**
 * @brief Ejecuta la demostración concurrente con múltiples productores y consumidores (thread-demo).
 */
int thread_demo_run(int num_producers, int num_consumers, int total_items);

/**
 * @brief Obtiene las estadísticas de la última ejecución de hilos (thread-stats).
 */
const thread_stats_t *thread_get_stats(void);

#endif // PRODUCER_CONSUMER_H
