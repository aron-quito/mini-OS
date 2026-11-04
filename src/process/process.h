#ifndef PROCESS_H
#define PROCESS_H

#define MAX_BG_PROCESSES 64
#define DEFAULT_BG_MEM_KB 128   /* RAM simulada por defecto si no se especifica */

typedef enum {
    PROC_STATE_RUNNING,
    PROC_STATE_STOPPED,
    PROC_STATE_DONE
} proc_state_t;

typedef struct {
    int pid;
    int ppid;
    char command[256];
    int mem_kb;
    proc_state_t state;
} bg_process_t;

/**
 * @brief Inicializa la tabla de procesos y subsistema de procesos.
 */
void process_init(void);

/**
 * @brief Ejecuta un comando en foreground mediante fork/execvp/waitpid.
 *
 * @param argv  Arreglo de argumentos terminado en NULL.
 * @return Código de retorno del proceso hijo, o -1 si falla fork.
 */
int process_execute_fg(char **argv);

/**
 * @brief Ejecuta un comando en background (Issue 2.2 / SIS-43).
 *
 * Lanza el proceso de forma asíncrona: el padre NO bloquea con waitpid.
 * Reserva memoria simulada con memory_alloc(). Si la RAM simulada es
 * insuficiente, encola la solicitud en mem_queue y retorna sin crear el proceso.
 *
 * @param argv    Arreglo de argumentos terminado en NULL (ya sin el argumento de KB).
 * @param mem_kb  Kilobytes de RAM simulada a reservar (mínimo DEFAULT_BG_MEM_KB).
 * @return PID del proceso hijo si se lanzó exitosamente, -1 si hubo error.
 */
int process_execute_bg(char **argv, int mem_kb);

/**
 * @brief Cosecha procesos en background que hayan terminado (no bloqueante).
 *
 * Debe llamarse justo antes de mostrar el prompt en shell_loop para evitar
 * el solapamiento entre la notificación de fin y la línea de entrada del usuario.
 * Usa waitpid(-1, &status, WNOHANG) en loop hasta que no queden hijos terminados.
 * Por cada proceso recogido, llama a memory_free() y muestra el informe final.
 */
void process_reap_bg(void);

/**
 * @brief Muestra la tabla de procesos de la sesión actual (comando ps-mini).
 */
void process_list_mini(void);

#endif // PROCESS_H
