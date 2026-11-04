#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#include "process.h"
#include "signals.h"
#include "../memory/memory.h"
#include "../memory/mem_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

/* =========================================================================
 * Tabla de procesos en background (Issue 2.3 / SIS-44)
 *
 * Array estático de BgProcess. Cada slot puede estar libre (used=0) u ocupado
 * (used=1). Un slot sigue "ocupado" incluso cuando el proceso termina para
 * que ps-mini pueda mostrar el historial completo de la sesión.
 *
 * La distinción entre proceso activo y terminado se hace con el campo status:
 *   PROC_RUNNING  → hijo sigue ejecutándose.
 *   PROC_FINISHED → terminó normalmente (WIFEXITED).
 *   PROC_KILLED   → terminado por señal (WIFSIGNALED).
 * ========================================================================= */
static BgProcess s_bg_table[MAX_BG_PROCESSES];

/* -------------------------------------------------------------------------
 * process_init — Inicializa todos los slots de la tabla a su estado vacío.
 * Debe llamarse una sola vez desde shell_init().
 * ------------------------------------------------------------------------- */
void process_init(void) {
    for (int i = 0; i < MAX_BG_PROCESSES; i++) {
        s_bg_table[i].pid       = 0;
        s_bg_table[i].ppid      = 0;
        s_bg_table[i].command[0]= '\0';
        s_bg_table[i].mem_kb    = 0;
        s_bg_table[i].status    = PROC_RUNNING;
        s_bg_table[i].exit_code = 0;
        s_bg_table[i].used      = 0;
    }
}

/* -------------------------------------------------------------------------
 * bg_table_add — Registra un proceso recién lanzado en la tabla.
 *
 * Rellena un slot libre con todos los metadatos del proceso:
 *   - pid, ppid provienen del fork().
 *   - command se construye concatenando argv[].
 *   - mem_kb es lo reservado en la RAM simulada.
 *   - status se fija a PROC_RUNNING.
 *
 * @return Índice del slot asignado, o -1 si la tabla está llena.
 * ------------------------------------------------------------------------- */
static int bg_table_add(pid_t pid, pid_t ppid,
                        char **argv, int mem_kb) {
    for (int i = 0; i < MAX_BG_PROCESSES; i++) {
        if (!s_bg_table[i].used) {
            s_bg_table[i].pid       = pid;
            s_bg_table[i].ppid      = ppid;
            s_bg_table[i].mem_kb    = mem_kb;
            s_bg_table[i].status    = PROC_RUNNING;
            s_bg_table[i].exit_code = 0;
            s_bg_table[i].used      = 1;

            /* Construir la cadena de comando a partir de argv[] */
            s_bg_table[i].command[0] = '\0';
            for (int j = 0; argv[j] != NULL; j++) {
                if (j > 0) {
                    strncat(s_bg_table[i].command, " ",
                            sizeof(s_bg_table[i].command)
                            - strlen(s_bg_table[i].command) - 1);
                }
                strncat(s_bg_table[i].command, argv[j],
                        sizeof(s_bg_table[i].command)
                        - strlen(s_bg_table[i].command) - 1);
            }

            return i;
        }
    }
    return -1; /* Tabla llena */
}

/* -------------------------------------------------------------------------
 * bg_table_set_finished — Actualiza el estado de un proceso al finalizar.
 *
 * A diferencia de la versión anterior (que eliminaba el slot), este diseño
 * conserva el registro para que ps-mini pueda mostrar el historial completo
 * de la sesión (tanto RUNNING como FINISHED/KILLED).
 *
 * @param pid       PID del proceso cosechado.
 * @param status    Valor raw de waitpid(); se analiza con WIFEXITED/WIFSIGNALED.
 * @return mem_kb del proceso si se encontró el PID, 0 si no estaba registrado.
 * ------------------------------------------------------------------------- */
static int bg_table_set_finished(pid_t pid, int wait_status) {
    for (int i = 0; i < MAX_BG_PROCESSES; i++) {
        if (s_bg_table[i].used && s_bg_table[i].pid == pid) {
            int mem_kb = s_bg_table[i].mem_kb;

            if (WIFEXITED(wait_status)) {
                s_bg_table[i].status    = PROC_FINISHED;
                s_bg_table[i].exit_code = WEXITSTATUS(wait_status);
            } else if (WIFSIGNALED(wait_status)) {
                s_bg_table[i].status    = PROC_KILLED;
                s_bg_table[i].exit_code = WTERMSIG(wait_status);
            }

            /* mem_kb se pone a 0 para indicar que ya fue liberada */
            s_bg_table[i].mem_kb = 0;

            return mem_kb;
        }
    }
    return 0; /* PID no estaba en la tabla (proceso de foreground u otro) */
}

/* =========================================================================
 * resolve_command_path — Verifica si argv[0] es ejecutable en el PATH.
 *
 * Una shell real (Bash/Zsh) NUNCA llama a fork() para un comando que no
 * existe. En su lugar recorre $PATH buscando el binario ANTES de crear
 * ningún proceso hijo. Esta función implementa esa misma lógica:
 *
 *  - Si el nombre contiene '/' (ruta absoluta o relativa), verifica con
 *    access(X_OK) que el archivo existe y tiene permiso de ejecución.
 *  - Si es un nombre simple (ej. "ls"), recorre cada directorio de $PATH
 *    separado por ':' y busca "<dir>/<nombre>" ejecutable.
 *
 * NOTA: Esta implementación tiene una limitación conocida (TOCTOU). Ver
 * docs/recomendaciones.md ítem 1 para la solución profesional con pipe.
 *
 * @param name  El nombre o ruta del programa (argv[0]).
 * @return 1 si se encontró un ejecutable válido, 0 en caso contrario.
 * ========================================================================= */
static int resolve_command_path(const char *name) {
    const char *path_env;
    char path_copy[4096];
    char candidate[4096];
    char *saveptr;
    char *dir;
    int  needed;

    /* Rutas absolutas o relativas: verificar directamente */
    if (strchr(name, '/')) {
        return access(name, X_OK) == 0;
    }

    /* Nombre simple: recorrer $PATH buscando "<dir>/<name>" */
    path_env = getenv("PATH");
    if (!path_env) {
        return 0;
    }

    /* Trabajar sobre una copia ya que strtok_r modifica la cadena */
    strncpy(path_copy, path_env, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    saveptr = NULL;
    dir = strtok_r(path_copy, ":", &saveptr);

    while (dir != NULL) {
        /* Construir ruta candidata "<dir>/<name>" */
        needed = snprintf(candidate, sizeof(candidate), "%s/%s", dir, name);
        if (needed > 0 && (size_t)needed < sizeof(candidate)) {
            if (access(candidate, X_OK) == 0) {
                return 1;
            }
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    return 0; /* No encontrado en ninguna carpeta del PATH */
}

/* =========================================================================
 * process_execute_fg — Ejecuta un comando externo en primer plano.
 *
 * Implementa el patrón correcto de una shell real (Issue 2.1 / SIS-42):
 *  0. resolve_command_path(): Verifica que el binario existe en $PATH ANTES
 *     de crear ningún proceso. Si no existe, reporta error y retorna 127.
 *  1. fork(): Solo se invoca si el binario fue confirmado como ejecutable.
 *  2. execvp() [en el hijo]: Transforma al proceso hijo en el programa destino.
 *  3. waitpid() [en el padre]: Bloquea hasta que el hijo finalice.
 *
 * @param argv  Arreglo terminado en NULL (argv[0]=comando, argv[1..n]=args).
 * @return Código de retorno del proceso hijo, o -1 si falla fork/waitpid.
 * ========================================================================= */
int process_execute_fg(char **argv) {
    pid_t pid;
    int   status;
    int   exit_code;
    int   term_sig;

    if (!argv || !argv[0]) {
        return -1;
    }

    /* Verificación previa al fork: no crear PIDs para comandos inexistentes */
    if (!resolve_command_path(argv[0])) {
        fprintf(stderr, "miniOS: comando no encontrado: '%s'\n", argv[0]);
        return 127;
    }

    pid = fork();

    if (pid < 0) {
        perror("miniOS: error al crear proceso hijo (fork)");
        return -1;
    }

    if (pid == 0) {
        /* ---------------------------------------------------------------
         * PROCESO HIJO
         * Restaurar SIGINT al comportamiento por defecto y ejecutar binario.
         * --------------------------------------------------------------- */
        signal(SIGINT, SIG_DFL);
        printf("[Proceso lanzado] PID: %d | PPID: %d (miniOS)\n\n", getpid(), getppid());
        fflush(stdout);

        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "miniOS: error al ejecutar '%s': %s\n", argv[0], strerror(errno));
            exit(126);
        }
    } else {
        /* ---------------------------------------------------------------
         * PROCESO PADRE — Espera bloqueante hasta que el hijo termine.
         * --------------------------------------------------------------- */
        signals_set_fg_pid(pid);
        pid_t w;
        do {
            w = waitpid(pid, &status, 0);
        } while (w < 0 && errno == EINTR);
        signals_set_fg_pid(0);

        if (w < 0) {
            perror("miniOS: error en waitpid");
            return -1;
        }

        printf("\n");

        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
            printf("[PID %d] FINALIZADO | Código de salida: %d\n", pid, exit_code);
            return exit_code;
        } else if (WIFSIGNALED(status)) {
            term_sig = WTERMSIG(status);
            printf("[PID %d] TERMINADO POR SEÑAL | Señal: %d\n", pid, term_sig);
            return 128 + term_sig;
        }
    }

    return 0;
}

/* =========================================================================
 * process_execute_bg — Ejecuta un comando en segundo plano (Issue 2.2 / SIS-43).
 *
 * Flujo completo:
 *  1. Verificar que el binario existe en $PATH (igual que fg, sin fork innecesario).
 *  2. Intentar reservar mem_kb en la RAM simulada con memory_alloc().
 *     - Si falla (RAM insuficiente): emitir alerta, encolar en mem_queue y salir.
 *     - Si tiene éxito: continuar con el fork.
 *  3. fork() + execvp() en el hijo.
 *  4. El padre NO llama a waitpid() bloqueante: retorna inmediatamente al shell.
 *  5. El padre registra el PID en bg_table con metadatos completos (Issue 2.3).
 *
 * La notificación de fin del proceso se muestra más adelante en process_reap_bg(),
 * que se llama justo antes de cada prompt para evitar solapamientos.
 *
 * @param argv    Arreglo terminado en NULL. El argumento de KB ya fue extraído
 *                por shell_dispatch antes de llamar a esta función.
 * @param mem_kb  Kilobytes de RAM simulada a reservar para este proceso.
 * @return PID del hijo si se lanzó, -1 en error, 0 si fue encolado por RAM.
 * ========================================================================= */
int process_spawn_bg(char **argv, int mem_kb) {
    pid_t pid;
    int   slot;

    if (!argv || !argv[0]) {
        return -1;
    }

    if (!resolve_command_path(argv[0])) {
        fprintf(stderr, "miniOS: comando no encontrado: '%s'\n", argv[0]);
        return 127;
    }

    pid = fork();

    if (pid < 0) {
        perror("miniOS: error al crear proceso hijo en background (fork)");
        return -1;
    }

    if (pid == 0) {
        /* PROCESO HIJO — Aislar en su propio grupo de procesos para que Ctrl+C no lo mate */
        setpgid(0, 0);
        signal(SIGINT, SIG_IGN);
        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "miniOS: error al ejecutar '%s': %s\n", argv[0], strerror(errno));
            exit(126);
        }
    } else {
        /* PROCESO PADRE — Asignar RAM directamente al PID real */
        printf("[Proceso lanzado] PID: %d | PPID: %d (miniOS)\n", pid, getpid());

        if (memory_is_first_fit()) {
            ff_alloc(pid, mem_kb);
        } else {
            pag_alloc(pid, mem_kb);
        }

        slot = bg_table_add(pid, getpid(), argv, mem_kb);

        if (slot < 0) {
            fprintf(stderr,
                    "[ADVERTENCIA] bg_table llena (%d slots). "
                    "El proceso PID %d correrá pero no aparecerá en ps-mini.\n",
                    MAX_BG_PROCESSES, pid);
        }

        printf("[RAM ALLOC] PID %d asignado en RAM simulada: %d KB\n", pid, mem_kb);
        printf("[BG] Proceso '%s' corriendo en background | PID: %d | RAM: %d KB\n\n",
               argv[0], pid, mem_kb);
    }

    return pid;
}

int process_execute_bg(char **argv, int mem_kb) {
    if (!argv || !argv[0]) {
        return -1;
    }

    /* 1. Verificación previa al fork */
    if (!resolve_command_path(argv[0])) {
        fprintf(stderr, "miniOS: comando no encontrado: '%s'\n", argv[0]);
        return 127;
    }

    /* 2. Verificación de capacidad de RAM */
    int can_alloc = memory_can_allocate(mem_kb);
    if (can_alloc == -1) {
        /* Rechazo permanente: mem_kb supera la RAM total */
        fprintf(stderr,
            "[ERROR] '%s' rechazado: solicita %d KB pero la RAM total es %d KB.\n"
            "        Este proceso NUNCA podrá ejecutarse con la RAM actual.\n",
            argv[0], mem_kb, mem_state.total_ram_kb);
        return 1;
    }

    if (can_alloc == 0) {
        /* RAM insuficiente temporalmente: encolar para lanzamiento posterior */
        int argc = 0;
        while (argv[argc] != NULL) argc++;

        char cmd_display[128] = "";
        for (int i = 0; i < argc; i++) {
            if (i > 0) strncat(cmd_display, " ", sizeof(cmd_display) - strlen(cmd_display) - 1);
            strncat(cmd_display, argv[i], sizeof(cmd_display) - strlen(cmd_display) - 1);
        }

        printf("[ALERTA] Memoria RAM insuficiente para asignar %d KB al proceso '%s'.\n",
               mem_kb, argv[0]);
        int pos = mem_queue_enqueue_launch(argv, argc, mem_kb, cmd_display);
        if (pos < 0) {
            fprintf(stderr, "[ERROR] No se pudo encolar el proceso (error de memoria).\n");
            return 1;
        }
        printf("[QUEUE] Proceso en background '%s' encolado (Posición: %d).\n"
               "        Se ejecutará automáticamente cuando haya memoria disponible.\n",
               cmd_display, pos);
        return 0;
    }

    /* 3. Hay memoria disponible: bifurcar y ejecutar directamente */
    return process_spawn_bg(argv, mem_kb);
}

/* =========================================================================
 * process_reap_bg — Cosecha procesos en background que hayan terminado.
 *
 * Debe llamarse justo ANTES de imprimir el prompt en shell_loop().
 * Usa waitpid(-1, &status, WNOHANG) en un loop hasta que no queden
 * hijos terminados que recoger. Por cada proceso cosechado:
 *   - Actualiza su estado en la bg_table (PROC_FINISHED o PROC_KILLED).
 *   - Llama a memory_free() para liberar su RAM simulada.
 *   - Imprime el informe de finalización en una línea limpia.
 *
 * Sin esta función, la notificación de fin llegaría via SIGCHLD en
 * medio de la entrada del usuario, solapándose con el texto que escribe.
 * ========================================================================= */
static void handle_reaped_bg_process(pid_t pid, int status) {
    /* Actualizar estado en la tabla y recuperar los KB que tenía */
    int mem_kb = bg_table_set_finished(pid, status);

    /* Imprimir la notificación en una línea limpia antes del prompt */
    if (WIFEXITED(status)) {
        printf("\n[BG PID %d] FINALIZADO | Código de salida: %d | RAM liberada: %d KB\n",
               pid, WEXITSTATUS(status), mem_kb);
    } else if (WIFSIGNALED(status)) {
        printf("\n[BG PID %d] TERMINADO POR SEÑAL %d | RAM liberada: %d KB\n",
               pid, WTERMSIG(status), mem_kb);
    }

    /* Liberar la RAM simulada asociada a este proceso */
    if (mem_kb > 0) {
        memory_free(pid);
    }
}

void process_reap_bg(void) {
    int status;
    pid_t pid;

    /* 1. Drenar hijos ya cosechados asíncronamente por sigchld_handler (Issue 8.1) */
    while (signals_pop_reaped(&pid, &status)) {
        handle_reaped_bg_process(pid, status);
    }

    /* 2. Respaldo no bloqueante con waitpid(-1, &status, WNOHANG) */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        handle_reaped_bg_process(pid, status);
    }
}

/* =========================================================================
 * process_list_mini — Muestra la tabla de procesos de la sesión (ps-mini).
 *
 * Issue 2.4 (SIS-45): Imprime en formato tabular todos los slots ocupados
 * de la bg_table (tanto RUNNING como FINISHED/KILLED). Las columnas son:
 *   #, PID, PPID, Comando, RAM (KB), Estado.
 *
 * Formato de salida (alineado con la spec de Issue 2.4):
 *   ╔════╦════════╦════════╦══════════════════════╦══════════════╦══════════════╗
 *   ║ #  ║ PID    ║ PPID   ║ Comando              ║ RAM (KB)     ║ Estado       ║
 *   ╠════╬════════╬════════╬══════════════════════╬══════════════╬══════════════╣
 *   ║ 1  ║ 24891  ║ 24800  ║ sleep 10             ║ 256 KB       ║ RUNNING      ║
 *   ║ 2  ║ 24895  ║ 24800  ║ ls -la               ║ 0 KB         ║ DONE (0)     ║
 *   ╚════╩════════╩════════╩══════════════════════╩══════════════╩══════════════╝
 * ========================================================================= */
void process_list_mini(void) {
    int        total;
    int        i;
    int        row;
    BgProcess *p;
    char       estado[24];
    char       ram_str[16];
    char       cmd_display[21];

    /* Contar cuántos slots están en uso para detectar tabla vacía */
    total = 0;
    for (i = 0; i < MAX_BG_PROCESSES; i++) {
        if (s_bg_table[i].used) total++;
    }

    if (total == 0) {
        printf("[ps-mini] No hay procesos en background registrados en esta sesión.\n");
        return;
    }

    /* ---- Cabecera de la tabla ---- */
    printf("╔════╦════════╦════════╦══════════════════════╦══════════════╦══════════════╗\n");
    printf("║ %-2s ║ %-6s ║ %-6s ║ %-20s ║ %-12s ║ %-12s ║\n",
           "#", "PID", "PPID", "Comando", "RAM (KB)", "Estado");
    printf("╠════╬════════╬════════╬══════════════════════╬══════════════╬══════════════╣\n");

    /* ---- Filas de datos ---- */
    row = 1;
    for (i = 0; i < MAX_BG_PROCESSES; i++) {
        if (!s_bg_table[i].used) continue;

        p = &s_bg_table[i];

        /* Columna Estado — switch exhaustivo sobre ProcessStatus */
        switch (p->status) {
            case PROC_RUNNING:
                snprintf(estado, sizeof(estado), "RUNNING");
                break;
            case PROC_FINISHED:
                snprintf(estado, sizeof(estado), "DONE (%d)", p->exit_code);
                break;
            case PROC_KILLED:
                snprintf(estado, sizeof(estado), "KILLED (%d)", p->exit_code);
                break;
            default:
                snprintf(estado, sizeof(estado), "UNKNOWN");
                break;
        }

        /* Columna RAM: si ya se liberó (mem_kb==0 y no RUNNING), mostrar 0 */
        if (p->status == PROC_RUNNING) {
            snprintf(ram_str, sizeof(ram_str), "%d KB", p->mem_kb);
        } else {
            snprintf(ram_str, sizeof(ram_str), "0 KB");
        }

        /* Truncar comando si supera el ancho de columna */
        snprintf(cmd_display, sizeof(cmd_display), "%s", p->command);

        printf("║ %-2d ║ %-6d ║ %-6d ║ %-20s ║ %-12s ║ %-12s ║\n",
               row, (int)p->pid, (int)p->ppid,
               cmd_display, ram_str, estado);
        row++;
    }

    /* ---- Pie de la tabla ---- */
    printf("╚════╩════════╩════════╩══════════════════════╩══════════════╩══════════════╝\n");
}
