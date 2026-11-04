#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

/**
 * @file signals.c
 * @brief Manejadores de señales POSIX con sigaction (Issue 8.1 / SIS-64).
 *
 * - SIGCHLD: Recoge inmediatamente los hijos en background finalizados mediante
 *   waitpid(-1, &status, WNOHANG) dentro de un búfer circular async-signal-safe,
 *   erradicando procesos zombies (<defunct>) sin invocar funciones no reentrantes
 *   (como printf/malloc/free) dentro del contexto de interrupción.
 * - SIGINT (Ctrl+C): Protege el ciclo REPL principal para que no termine al
 *   presionar Ctrl+C, reimprimiendo limpiamente una nueva línea y el prompt.
 */

#include "signals.h"
#include "process.h"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/* Capacidad del búfer circular async-signal-safe de hijos cosechados */
#define REAP_RING_SIZE 64

typedef struct {
    pid_t pid;
    int   status;
} ReapedChild;

static volatile ReapedChild s_reap_ring[REAP_RING_SIZE];
static volatile sig_atomic_t s_reap_head = 0;
static volatile sig_atomic_t s_reap_tail = 0;

/* PID del proceso actualmente en foreground (0 cuando estamos en el prompt) */
static volatile sig_atomic_t s_fg_pid = 0;

void signals_set_fg_pid(pid_t pid) {
    s_fg_pid = (sig_atomic_t)pid;
}

/**
 * @brief Manejador async-signal-safe para SIGCHLD.
 *
 * Cosecha cualquier proceso hijo terminado en background con waitpid(-1, &status, WNOHANG)
 * evitando que permanezca en estado <defunct> mientras el usuario está en el prompt.
 */
static void sigchld_handler(int sig) {
    (void)sig;
    int saved_errno = errno;

    /* Si hay un proceso en foreground esperando en waitpid(fg_pid), no interceptarlo aquí */
    if (s_fg_pid == 0) {
        int status = 0;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            sig_atomic_t next_head = (s_reap_head + 1) % REAP_RING_SIZE;
            if (next_head != s_reap_tail) {
                s_reap_ring[s_reap_head].pid    = pid;
                s_reap_ring[s_reap_head].status = status;
                s_reap_head = next_head;
            }
        }
    }

    errno = saved_errno;
}

/**
 * @brief Manejador async-signal-safe para SIGINT (Ctrl+C).
 *
 * Evita que el shell principal termine. Si estamos esperando en el prompt
 * (s_fg_pid == 0), imprime salto de línea y "miniOS> " con write().
 */
static void sigint_handler(int sig) {
    (void)sig;
    int saved_errno = errno;

    if (s_fg_pid == 0) {
        const char msg[] = "\nminiOS> ";
        ssize_t r = write(STDOUT_FILENO, msg, sizeof(msg) - 1);
        (void)r;
    } else {
        const char nl[] = "\n";
        ssize_t r = write(STDOUT_FILENO, nl, sizeof(nl) - 1);
        (void)r;
    }

    errno = saved_errno;
}

int signals_pop_reaped(pid_t *out_pid, int *out_status) {
    if (s_reap_tail == s_reap_head) {
        return 0;
    }

    sig_atomic_t idx = s_reap_tail;
    if (out_pid != NULL) {
        *out_pid = s_reap_ring[idx].pid;
    }
    if (out_status != NULL) {
        *out_status = s_reap_ring[idx].status;
    }
    s_reap_tail = (idx + 1) % REAP_RING_SIZE;
    return 1;
}

void signals_init(void) {
    struct sigaction sa_chld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_handler = sigchld_handler;
    sa_chld.sa_flags   = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);

    struct sigaction sa_int;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_handler = sigint_handler;
    sa_int.sa_flags   = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);
}
