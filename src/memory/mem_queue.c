#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem_queue.h"

static mem_queue_node_t *s_head = NULL;
static mem_queue_node_t *s_tail = NULL;
static int s_count = 0;

void mem_queue_init(void) {
    mem_queue_clear();
}

int mem_queue_enqueue(int pid, int size_kb, const char *command) {
    mem_queue_node_t *node = (mem_queue_node_t *)malloc(sizeof(mem_queue_node_t));
    if (!node) return -1;

    node->pid = pid;
    node->size_kb = size_kb;
    strncpy(node->command, command ? command : "unknown", sizeof(node->command) - 1);
    node->command[sizeof(node->command) - 1] = '\0';
    node->enqueued_at = time(NULL);
    node->next = NULL;

    if (!s_tail) {
        s_head = s_tail = node;
    } else {
        s_tail->next = node;
        s_tail = node;
    }
    s_count++;
    return 0;
}

mem_queue_node_t *mem_queue_dequeue(void) {
    if (!s_head) return NULL;

    mem_queue_node_t *node = s_head;
    s_head = s_head->next;
    if (!s_head) s_tail = NULL;
    s_count--;
    return node;
}

mem_queue_node_t *mem_queue_peek(void) {
    return s_head;
}

int mem_queue_length(void) {
    return s_count;
}

void mem_queue_print(void) {
    printf("  [Cola de espera RAM]: %d proceso(s) en espera.\n", s_count);
}

void mem_queue_clear(void) {
    mem_queue_node_t *curr = s_head;
    while (curr) {
        mem_queue_node_t *tmp = curr->next;
        free(curr);
        curr = tmp;
    }
    s_head = s_tail = NULL;
    s_count = 0;
}
