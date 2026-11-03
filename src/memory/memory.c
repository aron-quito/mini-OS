/**
 * @file memory.c
 * @brief Implementación del subsistema de memoria física simulada.
 *
 * Issue 3.1 (SIS-46): Modela la RAM como una lista enlazada ordenada por start_kb,
 * arrancando con un único bloque gigante libre que cubre toda la memoria.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

/* Estado global de la memoria física simulada (Issue 3.1) */
MemoryState mem_state;

/* Configuración interna por defecto (128 KB RAM, páginas de 4 KB, First-Fit) */
static mem_config_t s_mem_config = {DEFAULT_RAM_KB, DEFAULT_PAGE_KB, 32, MEM_ALGO_FIRST_FIT};

/**
 * @brief Regenera la lista enlazada con 1 bloque gigante libre (Issue 3.1).
 *
 * 1. Libera nodos existentes para evitar fugas de memoria (memory leaks).
 * 2. Asigna un bloque que abarca desde 0 hasta total_ram_kb con status BLOCK_FREE.
 */
void mem_reset_blocks(void) {
    // 1. Liberar nodos previos de la lista
    MemBlock *current = mem_state.block_list;
    while (current != NULL) {
        MemBlock *next = current->next;
        free(current);
        current = next;
    }
    mem_state.block_list = NULL;

    // 2. Crear el bloque inicial gigante (toda la RAM libre, id=0, start=0)
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

/**
 * @brief Inicializa mem_state aplicando valores predeterminados (Issue 3.1).
 */
void mem_init(void) {
    // Algoritmo por defecto: First-Fit contiguo ("ff")
    strcpy(mem_state.algorithm, "ff");
    mem_state.page_size_kb = s_mem_config.page_size_kb;
    mem_state.total_frames = s_mem_config.total_frames;

    if (mem_state.total_ram_kb <= 0) {
        mem_state.total_ram_kb = s_mem_config.total_ram_kb;
    }

    // Inicializa la lista enlazada con el bloque gigante libre
    mem_reset_blocks();
}

/**
 * @brief Punto de entrada público para configurar e inicializar la memoria.
 *
 * Sincroniza s_mem_config y mem_state con el tamaño y algoritmo solicitado.
 */
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
    // Stub base — Se implementará en Issue 3.3 (ff_alloc con split)
    (void)pid;
    (void)size_kb;
    return 0;
}

int memory_free(int pid) {
    // Stub base — Se implementará en Issue 3.4 (ff_free con coalescing)
    (void)pid;
    return 0;
}

void memory_view(void) {
    // Stub base — Se implementará en Issue 6.1 (comando mem-view)
}

/**
 * @brief Limpia todas las estructuras dinámicas al apagar el simulador.
 */
void memory_cleanup(void) {
    MemBlock *current = mem_state.block_list;
    while (current != NULL) {
        MemBlock *next = current->next;
        free(current);
        current = next;
    }
    mem_state.block_list = NULL;
}
