#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

// Instancia de la variable global requerida por la Issue 3.1
MemoryState mem_state;

static mem_config_t s_mem_config = {DEFAULT_RAM_KB, DEFAULT_PAGE_KB, 32, MEM_ALGO_FIRST_FIT};

void mem_reset_blocks(void) {
    // 1. Liberar nodos previos de la lista para evitar fugas de memoria
    MemBlock *current = mem_state.block_list;
    while (current != NULL) {
        MemBlock *next = current->next;
        free(current);
        current = next;
    }
    mem_state.block_list = NULL;

    // 2. Crear el bloque inicial gigante (toda la RAM libre)
    MemBlock *initial_block = (MemBlock *)malloc(sizeof(MemBlock));
    if (initial_block == NULL) {
        perror("Error de asignacion de memoria en mem_reset_blocks");
        exit(EXIT_FAILURE);
    }

    initial_block->id = 0;
    initial_block->start_kb = 0;
    initial_block->size_kb = mem_state.total_ram_kb;
    initial_block->status = BLOCK_FREE;
    initial_block->process_id = -1;
    initial_block->next = NULL;

    // 3. Asignar como cabeza de la lista
    mem_state.block_list = initial_block;
}

void mem_init(void) {
    // Configuración por defecto del algoritmo contiguo
    strcpy(mem_state.algorithm, "ff");
    mem_state.page_size_kb = s_mem_config.page_size_kb;
    mem_state.total_frames = s_mem_config.total_frames;

    // Si no se asignó un tamaño de RAM, tomar el valor por defecto
    if (mem_state.total_ram_kb <= 0) {
        mem_state.total_ram_kb = s_mem_config.total_ram_kb;
    }

    // Inicializa la lista con el bloque libre gigante
    mem_reset_blocks();
}

void memory_init(int total_ram_kb, mem_algorithm_t algo) {
    s_mem_config.total_ram_kb = total_ram_kb;
    s_mem_config.active_algorithm = algo;

    mem_state.total_ram_kb = total_ram_kb;
    if (algo == MEM_ALGO_FIRST_FIT) {
        strcpy(mem_state.algorithm, "ff");
    } else {
        strcpy(mem_state.algorithm, "pag");
    }

    mem_init();
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
    // Liberar todos los nodos al destruir o cerrar la memoria
    MemBlock *current = mem_state.block_list;
    while (current != NULL) {
        MemBlock *next = current->next;
        free(current);
        current = next;
    }
    mem_state.block_list = NULL;
}
