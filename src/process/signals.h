#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

/**
 * @brief Configura manejadores POSIX seguros para SIGCHLD y SIGINT usando sigaction (Issue 8.1 / SIS-64).
 *
 * - SIGCHLD: Cosecha asíncrona con waitpid(-1, &status, WNOHANG) evitando procesos <defunct> (zombies).
 * - SIGINT (Ctrl+C): Evita que el shell principal muera y reimprime el prompt "miniOS> ".
 */
void signals_init(void);

/**
 * @brief Registra el PID del proceso actual en foreground (0 si el shell está en el prompt).
 *
 * Mientras fg_pid > 0, sigchld_handler delega la cosecha de ese PID a waitpid() en process_execute_fg().
 */
void signals_set_fg_pid(pid_t pid);

/**
 * @brief Extrae un proceso hijo en background cosechado asíncronamente por sigchld_handler.
 *
 * @param out_pid    Puntero donde almacenar el PID cosechado.
 * @param out_status Puntero donde almacenar el status raw de waitpid().
 * @return 1 si se extrajo un evento pendiente, 0 si la cola asíncrona está vacía.
 */
int signals_pop_reaped(pid_t *out_pid, int *out_status);

#endif // SIGNALS_H
