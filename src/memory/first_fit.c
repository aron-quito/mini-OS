#include <stdio.h>
#include <stdlib.h>
#include "memory/first_fit.h"

static ff_block_t *s_head = NULL;
static int s_total_kb = 0;

void first_fit_init(int total_kb) {
    first_fit_cleanup();
    s_total_kb = total_kb;
    s_head = (ff_block_t *)malloc(sizeof(ff_block_t));
    if (s_head) {
        s_head->start_kb = 0;
        s_head->size_kb = total_kb;
        s_head->is_free = 1;
        s_head->pid = 0;
        s_head->next = NULL;
    }
}

int first_fit_alloc(int pid, int size_kb) {
    (void)pid;
    (void)size_kb;
    // Stub inicial para Issue 1.1 (se implementará completamente en Issue 3.3)
    return 0;
}

int first_fit_free(int pid) {
    (void)pid;
    // Stub inicial para Issue 1.1 (se implementará completamente en Issue 3.4)
    return 0;
}

void first_fit_view(void) {
    printf("  [First-Fit]: Bloque inicial libre de %d KB\n", s_total_kb);
}

void first_fit_cleanup(void) {
    ff_block_t *curr = s_head;
    while (curr) {
        ff_block_t *tmp = curr->next;
        free(curr);
        curr = tmp;
    }
    s_head = NULL;
}
