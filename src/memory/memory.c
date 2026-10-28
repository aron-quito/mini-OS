#include "memory.h"

static mem_config_t s_mem_config = {DEFAULT_RAM_KB, DEFAULT_PAGE_KB, 32, MEM_ALGO_FIRST_FIT};

void memory_init(int total_ram_kb, mem_algorithm_t algo) {
    (void)total_ram_kb;
    (void)algo;
    // Stub base — Se implementará en Milestone 3 (SIS-46 a SIS-49)
}

const mem_config_t *memory_get_config(void) {
    return &s_mem_config;
}

int memory_alloc(int pid, int size_kb) {
    (void)pid;
    (void)size_kb;
    return 0;
}

int memory_free(int pid) {
    (void)pid;
    return 0;
}

void memory_view(void) {
    // Stub base — Se implementará en Issue 6.1 (SIS-57)
}

void memory_cleanup(void) {
    // Stub base
}
