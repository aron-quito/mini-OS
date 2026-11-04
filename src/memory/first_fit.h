#ifndef FIRST_FIT_H
#define FIRST_FIT_H

#include "memory.h"

/* Contador global de bloques para asignación de IDs únicos en splits */
extern int block_counter;

/**
 * @brief Asigna memoria bajo el esquema First-Fit con partición dinámica (split) — Issue 3.3.
 *
 * @param pid     PID del proceso solicitante.
 * @param size_kb Cantidad de KB solicitados.
 * @return 0 en éxito, -1 si no hay bloque libre con capacidad suficiente.
 */
int ff_alloc(int pid, int size_kb);

/**
 * @brief Libera los bloques de un PID y aplica coalescencia — Issue 3.4.
 * @return 0 en éxito, -1 si el PID no tiene memoria asignada.
 */
int ff_free(int pid);

/**
 * @brief Fusiona bloques libres adyacentes — Issue 3.4.
 * @return Número de fusiones realizadas.
 */
int merge_adjacent_blocks(void);

/**
 * @brief Inicializa la memoria bajo el esquema First-Fit con 1 único bloque libre gigante.
 */
void first_fit_init(int total_kb);

/**
 * @brief Alias de ff_alloc para compatibilidad.
 */
int first_fit_alloc(int pid, int size_kb);

/**
 * @brief Libera los bloques asociados a un PID y aplica coalescencia adyacente (Issue 3.4).
 */
int first_fit_free(int pid);

/**
 * @brief Visualiza los bloques de memoria First-Fit (Issue 6.1).
 */
void first_fit_view(void);

/**
 * @brief Libera la lista enlazada de bloques de memoria.
 */
void first_fit_cleanup(void);

#endif // FIRST_FIT_H
