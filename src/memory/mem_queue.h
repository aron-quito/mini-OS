#ifndef MEM_QUEUE_H
#define MEM_QUEUE_H

#include <time.h>

typedef struct mem_queue_node {
    int pid;
    int size_kb;
    char command[128];
    time_t enqueued_at;
    struct mem_queue_node *next;
} mem_queue_node_t;

/**
 * @brief Inicializa la cola FIFO de procesos esperando memoria.
 */
void mem_queue_init(void);

/**
 * @brief Encola un proceso que no pudo obtener RAM.
 */
int mem_queue_enqueue(int pid, int size_kb, const char *command);

/**
 * @brief Desencola el primer proceso de la cola FIFO.
 */
mem_queue_node_t *mem_queue_dequeue(void);

/**
 * @brief Obtiene el primer proceso de la cola sin extraerlo.
 */
mem_queue_node_t *mem_queue_peek(void);

/**
 * @brief Muestra el estado de la cola de espera de memoria (mem-queue).
 */
void mem_queue_print(void);

/**
 * @brief Retorna la cantidad de procesos en cola.
 */
int mem_queue_length(void);

/**
 * @brief Vacía y libera todos los nodos de la cola.
 */
void mem_queue_clear(void);

#endif // MEM_QUEUE_H
