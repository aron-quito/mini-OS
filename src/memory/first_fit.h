#ifndef FIRST_FIT_H
#define FIRST_FIT_H

typedef struct ff_block {
    int start_kb;
    int size_kb;
    int is_free;
    int pid;
    struct ff_block *next;
} ff_block_t;

/**
 * @brief Inicializa la memoria bajo el esquema First-Fit con 1 único bloque libre gigante.
 */
void first_fit_init(int total_kb);

/**
 * @brief Asigna memoria buscando el primer bloque libre con capacidad suficiente y divide (split).
 */
int first_fit_alloc(int pid, int size_kb);

/**
 * @brief Libera los bloques asociados a un PID y aplica coalescencia adyacente.
 */
int first_fit_free(int pid);

/**
 * @brief Visualiza los bloques de memoria First-Fit.
 */
void first_fit_view(void);

/**
 * @brief Libera la lista enlazada de bloques de memoria.
 */
void first_fit_cleanup(void);

#endif // FIRST_FIT_H
