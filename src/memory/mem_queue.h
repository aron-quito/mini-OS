#ifndef MEM_QUEUE_H
#define MEM_QUEUE_H

#include <time.h>

/* -----------------------------------------------------------------------
 * mem_queue.h — Issue 4.1 (SIS-50)
 *
 * Cola FIFO de procesos que esperan memoria RAM disponible.
 * Cada nodo guarda el PID, el tamaño solicitado (KB), el comando
 * y la marca de tiempo de ingreso para calcular el tiempo de espera.
 * ----------------------------------------------------------------------- */

typedef struct mem_queue_node {
    int    pid;
    int    size_kb;
    char   command[128];
    time_t enqueued_at;
    struct mem_queue_node *next;
} mem_queue_node_t;

/**
 * @brief Inicializa la cola FIFO (debe llamarse una vez al inicio).
 */
void mem_queue_init(void);

/**
 * @brief Encola un proceso que no pudo obtener RAM.
 * @return Posición 1-based en la cola, o -1 si falla malloc.
 */
int mem_queue_enqueue(int pid, int size_kb, const char *command);

/**
 * @brief Desencola el primer proceso (FIFO). El llamador debe liberar el nodo.
 * @return Puntero al nodo extraído, o NULL si la cola está vacía.
 */
mem_queue_node_t *mem_queue_dequeue(void);

/**
 * @brief Consulta el primer proceso sin extraerlo.
 * @return Puntero al nodo frontal, o NULL si la cola está vacía.
 */
mem_queue_node_t *mem_queue_peek(void);

/**
 * @brief Elimina el nodo con el PID dado (libera internamente).
 * @return 0 si se encontró y eliminó, -1 si no existe.
 */
int mem_queue_remove(int pid);

/**
 * @brief Retorna la cantidad de procesos en cola.
 */
int mem_queue_length(void);

/**
 * @brief Vacía y libera todos los nodos de la cola.
 */
void mem_queue_clear(void);

/**
 * @brief Muestra la tabla de procesos en espera (comando mem-queue).
 */
void mem_queue_print(void);

/**
 * @brief Desencolado reactivo y asignación automática tras liberación (Issue 4.4 / SIS-53).
 *
 * Inspecciona la cola FIFO tras liberar memoria e intenta promover los procesos
 * al frente según el algoritmo activo ("ff" o "pag"), respetando el orden estricto
 * FIFO (Head-of-Line Blocking si el primero no cabe).
 */
void mem_queue_process_waiting(void);

#endif /* MEM_QUEUE_H */
