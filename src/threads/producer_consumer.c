#include <stdio.h>
#include <string.h>
#include "producer_consumer.h"
#include "buffer.h"

static thread_stats_t s_last_stats = {0, 0, 0, 0, 0.0};

int thread_demo_run(int num_producers, int num_consumers, int total_items) {
    s_last_stats.producers_count = num_producers;
    s_last_stats.consumers_count = num_consumers;
    s_last_stats.items_produced = total_items;
    s_last_stats.items_consumed = total_items;
    s_last_stats.execution_time_sec = 0.05;

    // Stub inicial para Issue 1.1 (se implementará en detalle en Issue 7.2 / 7.3)
    printf("[THREADS] Demostración de hilos configurada con %d productores y %d consumidores (%d ítems)\n",
           num_producers, num_consumers, total_items);
    return 0;
}

const thread_stats_t *thread_get_stats(void) {
    return &s_last_stats;
}
