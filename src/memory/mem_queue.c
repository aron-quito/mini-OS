#include <stddef.h>
#include "mem_queue.h"

void mem_queue_init(void) {
    // Stub base — Se implementará en Milestone 4 (SIS-50)
}

int mem_queue_enqueue(int pid, int size_kb, const char *command) {
    (void)pid;
    (void)size_kb;
    (void)command;
    return 0;
}

mem_queue_node_t *mem_queue_dequeue(void) {
    return NULL;
}

mem_queue_node_t *mem_queue_peek(void) {
    return NULL;
}

void mem_queue_print(void) {
}

int mem_queue_length(void) {
    return 0;
}

void mem_queue_clear(void) {
}
