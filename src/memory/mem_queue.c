/*
 * mem_queue.c — Issue 4.1 (SIS-50)
 *
 * Implementación de la cola FIFO de procesos en espera de memoria RAM.
 *
 * Estructura interna:
 *   - Lista enlazada simple con punteros front (cabeza) y rear (cola).
 *   - 'count' lleva el número de nodos; 'total_waiting_kb' acumula
 *     el total de KB solicitados por los procesos en espera.
 *
 * Complejidades:
 *   enqueue  → O(1)  (inserción al rear)
 *   dequeue  → O(1)  (extracción del front)
 *   remove   → O(n)  (búsqueda por PID)
 *   clear    → O(n)
 *   print    → O(n)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mem_queue.h"
#include "memory.h"
#include "paging.h"
#include "../process/process.h"

/* -----------------------------------------------------------------------
 * Estado interno de la cola (singleton estático)
 * ----------------------------------------------------------------------- */
static struct {
    mem_queue_node_t *front;
    mem_queue_node_t *rear;
    int               count;
    int               total_waiting_kb;
} q;

/* -----------------------------------------------------------------------
 * mem_queue_init
 * ----------------------------------------------------------------------- */
void mem_queue_init(void) {
    q.front            = NULL;
    q.rear             = NULL;
    q.count            = 0;
    q.total_waiting_kb = 0;
}

/* -----------------------------------------------------------------------
 * mem_queue_free_node
 * ----------------------------------------------------------------------- */
void mem_queue_free_node(mem_queue_node_t *node) {
    if (!node) return;
    if (node->argv) {
        for (int i = 0; i < node->argc; i++) {
            if (node->argv[i]) {
                free(node->argv[i]);
            }
        }
        free(node->argv);
        node->argv = NULL;
    }
    free(node);
}

/* -----------------------------------------------------------------------
 * mem_queue_enqueue_alloc
 * ----------------------------------------------------------------------- */
int mem_queue_enqueue_alloc(int pid, int size_kb, const char *command) {
    mem_queue_node_t *node = malloc(sizeof(mem_queue_node_t));
    if (!node) return -1;

    node->type        = MEM_REQ_ALLOC;
    node->pid         = pid;
    node->size_kb     = size_kb;
    node->argv        = NULL;
    node->argc        = 0;
    node->enqueued_at = time(NULL);
    node->next        = NULL;
    strncpy(node->command, command ? command : "", sizeof(node->command) - 1);
    node->command[sizeof(node->command) - 1] = '\0';

    if (q.rear == NULL) {
        /* Cola vacía: front y rear apuntan al nuevo nodo */
        q.front = node;
        q.rear  = node;
    } else {
        q.rear->next = node;
        q.rear       = node;
    }

    q.count            += 1;
    q.total_waiting_kb += size_kb;
    return q.count;   /* Posición 1-based */
}

/* -----------------------------------------------------------------------
 * mem_queue_enqueue_launch
 * ----------------------------------------------------------------------- */
int mem_queue_enqueue_launch(char **argv, int argc, int size_kb, const char *cmd_display) {
    mem_queue_node_t *node = malloc(sizeof(mem_queue_node_t));
    if (!node) return -1;

    node->type        = MEM_REQ_LAUNCH;
    node->pid         = 0;
    node->size_kb     = size_kb;
    node->argc        = argc;
    node->argv        = malloc((argc + 1) * sizeof(char *));
    if (!node->argv) {
        free(node);
        return -1;
    }

    for (int i = 0; i < argc; i++) {
        node->argv[i] = strdup(argv[i]);
    }
    node->argv[argc] = NULL;

    node->enqueued_at = time(NULL);
    node->next        = NULL;
    strncpy(node->command, cmd_display ? cmd_display : "", sizeof(node->command) - 1);
    node->command[sizeof(node->command) - 1] = '\0';

    if (q.rear == NULL) {
        q.front = node;
        q.rear  = node;
    } else {
        q.rear->next = node;
        q.rear       = node;
    }

    q.count            += 1;
    q.total_waiting_kb += size_kb;
    return q.count;
}

/* -----------------------------------------------------------------------
 * mem_queue_enqueue (compatibilidad por defecto ALLOC)
 * ----------------------------------------------------------------------- */
int mem_queue_enqueue(int pid, int size_kb, const char *command) {
    return mem_queue_enqueue_alloc(pid, size_kb, command);
}

/* -----------------------------------------------------------------------
 * mem_queue_dequeue
 * ----------------------------------------------------------------------- */
mem_queue_node_t *mem_queue_dequeue(void) {
    if (q.front == NULL) return NULL;

    mem_queue_node_t *node = q.front;
    q.front = node->next;
    if (q.front == NULL) q.rear = NULL;   /* Cola quedó vacía */

    q.count            -= 1;
    q.total_waiting_kb -= node->size_kb;
    node->next          = NULL;
    return node;   /* El llamador debe hacer free() */
}

/* -----------------------------------------------------------------------
 * mem_queue_peek
 * ----------------------------------------------------------------------- */
mem_queue_node_t *mem_queue_peek(void) {
    return q.front;
}

/* -----------------------------------------------------------------------
 * mem_queue_remove  —  elimina el nodo con el PID dado
 * ----------------------------------------------------------------------- */
int mem_queue_remove(int pid) {
    mem_queue_node_t *prev = NULL;
    mem_queue_node_t *cur  = q.front;

    while (cur != NULL) {
        if (cur->pid == pid && cur->type == MEM_REQ_ALLOC) {
            /* Desencadenar el nodo */
            if (prev == NULL) {
                q.front = cur->next;
            } else {
                prev->next = cur->next;
            }
            if (cur == q.rear) q.rear = prev;

            q.count            -= 1;
            q.total_waiting_kb -= cur->size_kb;
            mem_queue_free_node(cur);
            return 0;
        }
        prev = cur;
        cur  = cur->next;
    }
    return -1;   /* PID no encontrado */
}

/* -----------------------------------------------------------------------
 * mem_queue_length
 * ----------------------------------------------------------------------- */
int mem_queue_length(void) {
    return q.count;
}

/* -----------------------------------------------------------------------
 * mem_queue_clear
 * ----------------------------------------------------------------------- */
void mem_queue_clear(void) {
    mem_queue_node_t *cur = q.front;
    while (cur != NULL) {
        mem_queue_node_t *tmp = cur->next;
        mem_queue_free_node(cur);
        cur = tmp;
    }
    q.front            = NULL;
    q.rear             = NULL;
    q.count            = 0;
    q.total_waiting_kb = 0;
}

/* -----------------------------------------------------------------------
 * mem_queue_print
 * ----------------------------------------------------------------------- */
void mem_queue_print(void) {
    printf("\n  +-----+--------+--------------------------+---------------------+\n");
    printf("  | Pos |  PID   | Comando                  | Tiempo en espera    |\n");
    printf("  +-----+--------+--------------------------+---------------------+\n");

    if (q.front == NULL) {
        printf("  |            (cola vacia — ningún proceso esperando)          |\n");
        printf("  +-----+--------+--------------------------+---------------------+\n");
        printf("  |  Procesos en espera: 0    Total KB solicitado: 0 KB         |\n");
        printf("  +-----+--------+--------------------------+---------------------+\n\n");
        return;
    }

    time_t now = time(NULL);
    mem_queue_node_t *cur = q.front;
    int pos = 1;

    while (cur != NULL) {
        long elapsed = (long)(now - cur->enqueued_at);
        int  mm      = (int)(elapsed / 60);
        int  ss      = (int)(elapsed % 60);

        char pid_str[16];
        if (cur->type == MEM_REQ_LAUNCH) {
            snprintf(pid_str, sizeof(pid_str), "PEND");
        } else {
            snprintf(pid_str, sizeof(pid_str), "%d", cur->pid);
        }

        printf("  | %-3d | %-6s | %-24s | %02d:%02d                |\n",
               pos, pid_str, cur->command, mm, ss);

        pos++;
        cur = cur->next;
    }

    printf("  +-----+--------+--------------------------+---------------------+\n");
    printf("  |  Procesos en espera : %-5d   Total KB solicitado: %-6d KB |\n",
           q.count, q.total_waiting_kb);
    printf("  +-----+--------+--------------------------+---------------------+\n\n");
}

/* -----------------------------------------------------------------------
 * mem_queue_process_waiting — Issue 4.4 (SIS-53)
 *
 * Desencolado reactivo y asignación/lanzamiento automático tras liberación:
 *   1. Mientras la cola tenga elementos (q.count > 0), consulta el nodo al
 *      frente con mem_queue_peek() sin extraerlo.
 *   2. Verifica si cabe en memoria con memory_can_allocate(front->size_kb).
 *   3. Si no cabe: detiene el ciclo inmediatamente (break) para preservar
 *      el orden estricto FIFO (Head-of-Line Blocking).
 *   4. Si cabe:
 *      - MEM_REQ_ALLOC: asigna memoria simulada (ff_alloc / pag_alloc).
 *      - MEM_REQ_LAUNCH: bifurca y lanza el proceso real con process_spawn_bg,
 *        asignándole el PID real recién creado.
 *      - Desencola y libera el nodo.
 * ----------------------------------------------------------------------- */
void mem_queue_process_waiting(void) {
    while (q.count > 0) {
        mem_queue_node_t *front = mem_queue_peek();
        if (front == NULL) {
            break;
        }

        /* 1. Comprobar si cabe en memoria */
        int can_alloc = memory_can_allocate(front->size_kb);
        if (can_alloc != 1) {
            /* Head-of-Line Blocking: Si el proceso al frente no cabe,
             * se detiene para respetar el orden estricto FIFO. */
            break;
        }

        /* 2. Procesar según tipo */
        if (front->type == MEM_REQ_ALLOC) {
            int allocated = 0;
            if (memory_is_first_fit()) {
                allocated = (ff_alloc(front->pid, front->size_kb) == 0);
            } else {
                allocated = (pag_alloc(front->pid, front->size_kb) == 0);
            }

            if (allocated) {
                printf("[AUTO-ALLOC] Proceso en espera PID %d asignado exitosamente en RAM (%d KB).\n",
                       front->pid, front->size_kb);
                mem_queue_node_t *node = mem_queue_dequeue();
                mem_queue_free_node(node);
            } else {
                break;
            }
        } else if (front->type == MEM_REQ_LAUNCH) {
            printf("[AUTO-LAUNCH] Memoria disponible detectada (%d KB). Lanzando '%s' desde la cola...\n",
                   front->size_kb, front->command);

            int launched_pid = process_spawn_bg(front->argv, front->size_kb);
            if (launched_pid > 0) {
                mem_queue_node_t *node = mem_queue_dequeue();
                mem_queue_free_node(node);
            } else {
                fprintf(stderr, "[ERROR] Falló el lanzamiento del proceso en cola '%s'.\n",
                        front->command);
                mem_queue_node_t *node = mem_queue_dequeue();
                mem_queue_free_node(node);
            }
        }
    }
}
