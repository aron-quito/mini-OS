/**
 * @file first_fit.c
 * @brief Implementación del algoritmo First-Fit para particionamiento dinámico contiguo.
 *
 * Issue 3.3 (SIS-48): Búsqueda del primer bloque libre con capacidad suficiente
 * y partición dinámica (split) del excedente.
 */

#include <stdio.h>
#include <stdlib.h>
#include "first_fit.h"
#include "memory.h"
#include "mem_queue.h"

/* Contador global de bloques para IDs únicos (Issue 3.3) */
int block_counter = 0;

/**
 * @brief Asignación contigua First-Fit con partición dinámica (split) — Issue 3.3.
 *
 * Recorre secuencialmente mem_state.block_list buscando el primer bloque libre
 * donde size_kb quepa. Si el bloque es más grande que lo solicitado, realiza un split
 * creando un bloque excedente con status BLOCK_FREE y process_id = -1.
 *
 * @param pid     PID del proceso que solicita memoria.
 * @param size_kb Kilobytes requeridos (> 0).
 * @return 0 en éxito, -1 en fallo (memoria insuficiente o fragmentación externa).
 */
int ff_alloc(int pid, int size_kb) {
    if (size_kb <= 0) {
        fprintf(stderr, "[FF] Error: El tamaño de memoria solicitado debe ser positivo (> 0 KB)\n");
        return -1;
    }

    MemBlock *curr = mem_state.block_list;
    while (curr != NULL) {
        if (curr->status == BLOCK_FREE && curr->size_kb >= size_kb) {
            if (curr->size_kb > size_kb) {
                // Split del bloque: reservar memoria para el bloque restante
                MemBlock *resto = (MemBlock *)malloc(sizeof(MemBlock));
                if (resto == NULL) {
                    fprintf(stderr, "Error: Fallo de asignacion de memoria dinamica para MemBlock\n");
                    return -1;
                }
                resto->id = ++block_counter;
                resto->start_kb = curr->start_kb + size_kb;
                resto->size_kb = curr->size_kb - size_kb;
                resto->status = BLOCK_FREE;
                resto->process_id = -1;
                resto->next = curr->next;

                curr->size_kb = size_kb;
                curr->next = resto;
            }
            curr->status = BLOCK_USED;
            curr->process_id = pid;
            if (pid > 0) {
                printf("[FF] PID %d asignado en dirección %d KB (%d KB)\n", pid, curr->start_kb, size_kb);
            }
            return 0; // Asignación exitosa
        }
        curr = curr->next;
    }

    // Memoria insuficiente o fragmentación externa
    return -1;
}

void first_fit_init(int total_kb) {
    mem_state.total_ram_kb = total_kb;
    mem_reset_blocks();
}

int first_fit_alloc(int pid, int size_kb) {
    return ff_alloc(pid, size_kb);
}

/**
 * @brief Coalescencia: fusiona bloques libres adyacentes (Issue 3.4).
 *
 * Tras una fusión curr no avanza, de modo que varias fusiones consecutivas
 * se resuelven en una sola pasada.
 *
 * @return Cantidad de fusiones realizadas.
 */
int merge_adjacent_blocks(void) {
    int merges = 0;
    MemBlock *curr = mem_state.block_list;

    while (curr != NULL && curr->next != NULL) {
        if (curr->status == BLOCK_FREE && curr->next->status == BLOCK_FREE) {
            MemBlock *temp = curr->next;
            curr->size_kb += temp->size_kb;
            curr->next = temp->next;
            free(temp);
            merges++;
        } else {
            curr = curr->next;
        }
    }
    return merges;
}

/**
 * @brief Libera los bloques ocupados por un PID y aplica coalescencia (Issue 3.4).
 *
 * @param pid PID del proceso cuya memoria se libera.
 * @return 0 en éxito, -1 si el PID no tiene bloques asignados.
 */
int ff_free(int pid) {
    int found = 0;

    for (MemBlock *curr = mem_state.block_list; curr != NULL; curr = curr->next) {
        if (curr->status == BLOCK_USED && curr->process_id == pid) {
            curr->status = BLOCK_FREE;
            curr->process_id = -1;
            found++;
        }
    }

    if (found == 0) {
        fprintf(stderr, "[FF] Advertencia: el PID %d no tiene bloques asignados en memoria\n", pid);
        return -1;
    }

    int merged = merge_adjacent_blocks();
    printf("[FF] Memoria liberada para PID %d. Bloques fusionados: %d\n", pid, merged);

    /* Issue 4.4 (SIS-53): Intentar asignar automáticamente a procesos en espera */
    mem_queue_process_waiting();
    return 0;
}

int first_fit_free(int pid) {
    return ff_free(pid);
}

void first_fit_view(void) {
    // Stub base — Se implementará en Issue 6.1 (SIS-57)
}

void first_fit_cleanup(void) {
    mem_reset_blocks();
}
