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

typedef enum {
    MEM_REQ_ALLOC,   /* Solicitud pasiva de memoria simulada (mem-alloc <pid> <kb>) */
    MEM_REQ_LAUNCH   /* Solicitud de lanzamiento de proceso en background (cmd &) */
} mem_req_type_t;

typedef struct mem_queue_node {
    mem_req_type_t type;
    int            pid;            /* PID para ALLOC (o 0 para LAUNCH) */
    int            size_kb;
    char           command[128];
    char         **argv;           /* Argumentos clonados para LAUNCH (NULL para ALLOC) */
    int            argc;
    time_t         enqueued_at;
    struct mem_queue_node *next;
} mem_queue_node_t;

/**
 * @brief Inicializa la cola FIFO (debe llamarse una vez al inicio).
 */
void mem_queue_init(void);

/**
 * @brief Encola una solicitud pasiva de memoria simulada (ALLOC).
 * @return Posición 1-based en la cola, o -1 si falla malloc.
 */
int mem_queue_enqueue_alloc(int pid, int size_kb, const char *command);

/**
 * @brief Encola una solicitud de lanzamiento de proceso en background (LAUNCH).
 * @return Posición 1-based en la cola, o -1 si falla malloc.
 */
int mem_queue_enqueue_launch(char **argv, int argc, int size_kb, const char *cmd_display);

/**
 * @brief Encola un proceso que no pudo obtener RAM (mantiene compatibilidad, por defecto ALLOC).
 * @return Posición 1-based en la cola, o -1 si falla malloc.
 */
int mem_queue_enqueue(int pid, int size_kb, const char *command);

/**
 * @brief Desencola el primer proceso (FIFO). El llamador debe liberar con mem_queue_free_node().
 * @return Puntero al nodo extraído, o NULL si la cola está vacía.
 */
mem_queue_node_t *mem_queue_dequeue(void);

/**
 * @brief Libera un nodo extraído de la cola, liberando su argv si aplica.
 */
void mem_queue_free_node(mem_queue_node_t *node);

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
