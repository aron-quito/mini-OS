#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "first_fit.h"
#include "paging.h"
#include "mem_queue.h"

static mem_config_t s_mem_config;

void memory_init(int total_ram_kb, mem_algorithm_t algo) {
    s_mem_config.total_ram_kb = (total_ram_kb > 0) ? total_ram_kb : DEFAULT_RAM_KB;
    s_mem_config.page_size_kb = DEFAULT_PAGE_KB;
    s_mem_config.total_frames = s_mem_config.total_ram_kb / s_mem_config.page_size_kb;
    s_mem_config.active_algorithm = algo;

    first_fit_init(s_mem_config.total_ram_kb);
    paging_init(s_mem_config.total_ram_kb, s_mem_config.page_size_kb);
    mem_queue_init();
}

const mem_config_t *memory_get_config(void) {
    return &s_mem_config;
}

int memory_alloc(int pid, int size_kb) {
    if (s_mem_config.active_algorithm == MEM_ALGO_FIRST_FIT) {
        return first_fit_alloc(pid, size_kb);
    } else {
        return paging_alloc(pid, size_kb);
    }
}

int memory_free(int pid) {
    if (s_mem_config.active_algorithm == MEM_ALGO_FIRST_FIT) {
        return first_fit_free(pid);
    } else {
        return paging_free(pid);
    }
}

void memory_view(void) {
    printf("\n=== ESTADO DE MEMORIA RAM (%d KB Total) ===\n", s_mem_config.total_ram_kb);
    printf("Algoritmo activo: %s\n",
           (s_mem_config.active_algorithm == MEM_ALGO_FIRST_FIT) ? "First-Fit" : "Paginación");
    if (s_mem_config.active_algorithm == MEM_ALGO_FIRST_FIT) {
        first_fit_view();
    } else {
        paging_view();
    }
    mem_queue_print();
    printf("===========================================\n\n");
}

void memory_cleanup(void) {
    first_fit_cleanup();
    paging_cleanup();
    mem_queue_clear();
}
