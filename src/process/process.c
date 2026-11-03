#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#include "process.h"
#include "../memory/memory.h"
#include "../memory/mem_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

/* -----------------------------------------------------------------------
 * Tabla interna de procesos en background
 *
 * Registra los procesos lanzados con '&' para poder cosecharlos en
 * process_reap_bg() y asociar su PID con los KB de RAM reservados.
 * Issue 2.3 / SIS-44 expandirá esta tabla con estado, PPID y comando.
 * ----------------------------------------------------------------------- */
typedef struct {
    pid_t pid;
    int   mem_kb;
    int   used;         /* 1 = slot ocupado, 0 = libre */
} bg_slot_t;

static bg_slot_t s_bg_table[MAX_BG_PROCESSES];

void process_init(void) {
    for (int i = 0; i < MAX_BG_PROCESSES; i++) {
        s_bg_table[i].pid    = 0;
        s_bg_table[i].mem_kb = 0;
        s_bg_table[i].used   = 0;
    }
}

/* -----------------------------------------------------------------------
 * bg_table_add — Registra un proceso nuevo en la tabla interna.
 * Retorna 1 si hubo espacio, 0 si la tabla está llena.
 * ----------------------------------------------------------------------- */
static int bg_table_add(pid_t pid, int mem_kb) {
    for (int i = 0; i < MAX_BG_PROCESSES; i++) {
        if (!s_bg_table[i].used) {
            s_bg_table[i].pid    = pid;
            s_bg_table[i].mem_kb = mem_kb;
            s_bg_table[i].used   = 1;
            return 1;
        }
    }
    return 0; /* Tabla llena */
}

/* -----------------------------------------------------------------------
 * bg_table_remove — Elimina un PID de la tabla y retorna sus KB reservados.
 * Retorna mem_kb si se encontró el PID, 0 si no estaba registrado.
 * ----------------------------------------------------------------------- */
static int bg_table_remove(pid_t pid) {
    for (int i = 0; i < MAX_BG_PROCESSES; i++) {
        if (s_bg_table[i].used && s_bg_table[i].pid == pid) {
            int mem_kb = s_bg_table[i].mem_kb;
            s_bg_table[i].pid    = 0;
            s_bg_table[i].mem_kb = 0;
            s_bg_table[i].used   = 0;
            return mem_kb;
        }
    }
    return 0;
}

/* -----------------------------------------------------------------------
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
 * ----------------------------------------------------------------------- */
static int resolve_command_path(const char *name) {
    /* Rutas absolutas o relativas: verificar directamente */
    if (strchr(name, '/')) {
        return access(name, X_OK) == 0;
    }

    /* Nombre simple: recorrer $PATH buscando "<dir>/<name>" */
    const char *path_env = getenv("PATH");
    if (!path_env) {
        return 0;
    }

    /* Trabajar sobre una copia ya que strtok_r modifica la cadena */
    char path_copy[4096];
    strncpy(path_copy, path_env, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char candidate[4096];
    char *saveptr = NULL;
    char *dir = strtok_r(path_copy, ":", &saveptr);

    while (dir != NULL) {
        /* Construir ruta candidata "<dir>/<name>" */
        int needed = snprintf(candidate, sizeof(candidate), "%s/%s", dir, name);
        if (needed > 0 && (size_t)needed < sizeof(candidate)) {
            if (access(candidate, X_OK) == 0) {
                return 1;
            }
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    return 0; /* No encontrado en ninguna carpeta del PATH */
}

/* -----------------------------------------------------------------------
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
 * ----------------------------------------------------------------------- */
int process_execute_fg(char **argv) {
    if (!argv || !argv[0]) {
        return -1;
    }

    /* Verificación previa al fork: no crear PIDs para comandos inexistentes */
    if (!resolve_command_path(argv[0])) {
        fprintf(stderr, "miniOS: comando no encontrado: '%s'\n", argv[0]);
        return 127;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("miniOS: error al crear proceso hijo (fork)");
        return -1;
    }

    if (pid == 0) {
        /* ---------------------------------------------------------------
         * PROCESO HIJO
         * El binario ya fue validado; si execvp falla ahora es por otro
         * motivo (permisos revocados, etc.) → exit(126).
         * --------------------------------------------------------------- */
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
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            perror("miniOS: error en waitpid");
            return -1;
        }

        printf("\n");

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("[PID %d] FINALIZADO | Código de salida: %d\n", pid, exit_code);
            return exit_code;
        } else if (WIFSIGNALED(status)) {
            int term_sig = WTERMSIG(status);
            printf("[PID %d] TERMINADO POR SEÑAL | Señal: %d\n", pid, term_sig);
            return 128 + term_sig;
        }
    }

    return 0;
}

/* -----------------------------------------------------------------------
 * process_execute_bg — Ejecuta un comando en segundo plano (Issue 2.2 / SIS-43).
 *
 * Flujo completo:
 *  1. Verificar que el binario existe en $PATH (igual que fg, sin fork innecesario).
 *  2. Intentar reservar mem_kb en la RAM simulada con memory_alloc().
 *     - Si falla (RAM insuficiente): emitir alerta, encolar en mem_queue y salir.
 *     - Si tiene éxito: continuar con el fork.
 *  3. fork() + execvp() en el hijo.
 *  4. El padre NO llama a waitpid() bloqueante: retorna inmediatamente al shell.
 *  5. El padre registra el PID en la tabla interna (bg_table) vinculando mem_kb.
 *
 * La notificación de fin del proceso se muestra más adelante en process_reap_bg(),
 * que se llama justo antes de cada prompt para evitar solapamientos.
 *
 * @param argv    Arreglo terminado en NULL. El argumento de KB ya fue extraído
 *                por shell_dispatch antes de llamar a esta función.
 * @param mem_kb  Kilobytes de RAM simulada a reservar para este proceso.
 * @return PID del hijo si se lanzó, -1 en error, 0 si fue encolado por RAM.
 * ----------------------------------------------------------------------- */
int process_execute_bg(char **argv, int mem_kb) {
    if (!argv || !argv[0]) {
        return -1;
    }

    /* 1. Verificación previa al fork */
    if (!resolve_command_path(argv[0])) {
        fprintf(stderr, "miniOS: comando no encontrado: '%s'\n", argv[0]);
        return 127;
    }

    /*
     * 2. Reservar memoria simulada ANTES del fork.
     *
     * memory_alloc() es llamada con PID 0 como placeholder porque el PID real
     * del hijo aún no existe. Cuando el Milestone 3 implemente la RAM real,
     * esta llamada deberá refactorizarse para reasignar la entrada al PID real
     * del hijo (obtenido justo después del fork).
     *
     * Por ahora el stub siempre retorna 0 (éxito), por lo que el flujo de
     * "RAM insuficiente" y encolamiento está estructurado pero inactivo.
     */
    if (memory_alloc(0, mem_kb) != 0) {
        /* RAM simulada insuficiente: alertar y encolar sin crear proceso */
        const mem_config_t *cfg = memory_get_config();
        fprintf(stderr,
                "[ALERTA RAM] Memoria insuficiente para '%s' "
                "(Requerido: %d KB | RAM total simulada: %d KB).\n",
                argv[0], mem_kb, cfg->total_ram_kb);
        mem_queue_enqueue(0, mem_kb, argv[0]);
        printf("[QUEUE] Proceso '%s' retenido en cola de espera de RAM.\n", argv[0]);
        printf("[INFO] Se iniciará cuando otro proceso libere espacio.\n");
        return 0;
    }

    /* 3. Fork: solo si hay memoria disponible */
    pid_t pid = fork();

    if (pid < 0) {
        perror("miniOS: error al crear proceso hijo en background (fork)");
        memory_free(0); /* Liberar la reserva provisional */
        return -1;
    }

    if (pid == 0) {
        /* ---------------------------------------------------------------
         * PROCESO HIJO — Ejecutar en background.
         * El hijo no necesita imprimir nada de PID; esa info la muestra
         * el padre justo debajo antes de devolver el control al shell.
         * --------------------------------------------------------------- */
        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "miniOS: error al ejecutar '%s': %s\n", argv[0], strerror(errno));
            exit(126);
        }
    } else {
        /* ---------------------------------------------------------------
         * PROCESO PADRE — Retornar inmediatamente (asíncrono).
         * Registrar el PID en la tabla interna y mostrar el banner de BG.
         * NO se llama a waitpid() bloqueante aquí.
         * --------------------------------------------------------------- */
        bg_table_add(pid, mem_kb);

        printf("[Proceso lanzado] PID: %d | PPID: %d (miniOS)\n", pid, getpid());
        printf("[RAM ALLOC] PID %d asignado en RAM simulada: %d KB\n", pid, mem_kb);
        printf("[BG] Proceso '%s' corriendo en background | PID: %d | RAM: %d KB\n\n",
               argv[0], pid, mem_kb);
    }

    return pid;
}

/* -----------------------------------------------------------------------
 * process_reap_bg — Cosecha procesos en background que hayan terminado.
 *
 * Debe llamarse justo ANTES de imprimir el prompt en shell_loop().
 * Usa waitpid(-1, &status, WNOHANG) en un loop hasta que no queden
 * hijos terminados que recoger. Por cada proceso cosechado:
 *   - Elimina su entrada de la tabla interna (bg_table_remove).
 *   - Llama a memory_free() para liberar su RAM simulada.
 *   - Imprime el informe de finalización en una línea limpia.
 *
 * Sin esta función, la notificación de fin llegaría via SIGCHLD en
 * medio de la entrada del usuario, solapándose con el texto que escribe.
 * ----------------------------------------------------------------------- */
void process_reap_bg(void) {
    int status;
    pid_t pid;

    /*
     * WNOHANG: retorna 0 inmediatamente si no hay hijos terminados.
     * El loop recoge todos los que hayan finalizado desde la última llamada.
     */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int mem_kb = bg_table_remove(pid);

        /* Liberar la RAM simulada asociada a este proceso */
        if (mem_kb > 0) {
            memory_free(pid);
        }

        /* Imprimir la notificación en una línea limpia antes del prompt */
        if (WIFEXITED(status)) {
            printf("\n[BG PID %d] FINALIZADO | Código de salida: %d | RAM liberada: %d KB\n",
                   pid, WEXITSTATUS(status), mem_kb);
        } else if (WIFSIGNALED(status)) {
            printf("\n[BG PID %d] TERMINADO POR SEÑAL %d | RAM liberada: %d KB\n",
                   pid, WTERMSIG(status), mem_kb);
        }
    }
}

/* -----------------------------------------------------------------------
 * process_list_mini — Muestra la tabla de procesos en background (ps-mini).
 * Issue 2.4 (SIS-45): Imprime PID, RAM y estado de cada proceso en bg_table.
 * ----------------------------------------------------------------------- */
void process_list_mini(void) {
    // Stub base — Se implementará en Issue 2.4 (SIS-45)
}
