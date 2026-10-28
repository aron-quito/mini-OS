#include "producer_consumer.h"

static thread_stats_t s_stats = {0, 0, 0, 0, 0.0};

int thread_demo_run(int num_producers, int num_consumers, int total_items) {
    (void)num_producers;
    (void)num_consumers;
    (void)total_items;
    // Stub base — Se implementará en Milestone 7 (SIS-61 / SIS-62)
    return 0;
}

const thread_stats_t *thread_get_stats(void) {
    return &s_stats;
}
