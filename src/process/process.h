#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>  /* pid_t */

/* -------------------------------------------------------------------------
 * Límite máximo de procesos en background que la tabla puede registrar.
 * Issue 2.3 (SIS-44): valor configurable; la spec sugiere 32 como ejemplo.
 * ------------------------------------------------------------------------- */
#define MAX_BG_PROCESSES 32

/* RAM simulada por defecto (KB) si el usuario no especifica cantidad */
#define DEFAULT_BG_MEM_KB 128

/* -------------------------------------------------------------------------
 * ProcessStatus — Estado de un proceso en background.
 *
 * Nombres alineados con la especificación de Issue 2.3 / SIS-44:
 *   PROC_RUNNING   → el proceso hijo sigue corriendo.
 *   PROC_FINISHED  → terminó normalmente (WIFEXITED).
 *   PROC_KILLED    → terminado por señal (WIFSIGNALED).
 * ------------------------------------------------------------------------- */
typedef enum {
    PROC_RUNNING,
    PROC_FINISHED,
    PROC_KILLED
} ProcessStatus;

/* -------------------------------------------------------------------------
 * BgProcess — Registro completo de un proceso en background (Issue 2.3).
 *
 * Almacena toda la metadata necesaria para el comando ps-mini (Issue 2.4):
 *   pid       → identificador del proceso hijo.
 *   ppid      → identificador del proceso padre (el shell).
 *   command   → cadena del comando completo ejecutado (ej. "sleep 10").
 *   mem_kb    → kilobytes de RAM simulada reservados.
 *   status    → estado actual del proceso (RUNNING / FINISHED / KILLED).
 *   exit_code → código de salida o número de señal si aplica.
 *   used      → 1 = slot ocupado en la tabla, 0 = libre.
 * ------------------------------------------------------------------------- */
typedef struct {
    pid_t         pid;
    pid_t         ppid;
    char          command[128];
    int           mem_kb;
    ProcessStatus status;
    int           exit_code;
    int           used;         /* Bandera interna: 1=slot activo, 0=vacío */
} BgProcess;

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
 *
 * Issue 2.4 (SIS-45): Imprime en formato tabular todos los procesos registrados
 * en la sesión (tanto en ejecución como terminados), con columnas:
 * #, PID, PPID, Comando, RAM (KB), Estado.
 */
void process_list_mini(void);

#endif /* PROCESS_H */
