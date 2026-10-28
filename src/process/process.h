#ifndef PROCESS_H
#define PROCESS_H

#define MAX_BG_PROCESSES 64

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
 */
int process_execute_fg(char **argv);

/**
 * @brief Ejecuta un comando en background registrando memoria simulada.
 */
int process_execute_bg(char **argv, int mem_kb);

/**
 * @brief Muestra la tabla de procesos de la sesión actual (comando ps-mini).
 */
void process_list_mini(void);

#endif // PROCESS_H
