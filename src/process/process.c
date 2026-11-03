#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

void process_init(void) {
    // Stub base — Se implementará en Milestone 2 (SIS-42 a SIS-45)
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
            /* access(X_OK): el archivo existe y tiene permiso de ejecución */
            if (access(candidate, X_OK) == 0) {
                return 1;
            }
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    return 0; /* No encontrado en ninguna carpeta del PATH */
}

/* -----------------------------------------------------------------------
 * process_execute_fg — Ejecuta un comando externo en primer plano (foreground).
 *
 * Implementa el patrón correcto de una shell real (Issue 2.1 / SIS-42):
 *
 *  0. resolve_command_path(): Verifica que el binario existe en $PATH ANTES
 *     de crear ningún proceso. Si no existe, reporta el error y retorna
 *     inmediatamente sin llamar a fork(). Así se evita mostrar PIDs para
 *     comandos inexistentes, igual que hace Bash o Zsh.
 *  1. fork(): Solo se invoca si el binario fue confirmado como ejecutable.
 *  2. execvp() [en el hijo]: Transforma al proceso hijo en el programa destino
 *     pasándole el arreglo argv completo (nombre + todos los argumentos).
 *  3. waitpid() [en el padre]: Bloquea al shell padre hasta que el hijo finalice,
 *     capturando su código de salida (exit status) o señal de terminación.
 *
 * @param argv  Arreglo de cadenas terminado en NULL (argv[0]=comando, argv[1..n]=args).
 * @return Código de retorno del proceso hijo (0=éxito, >0=error), o -1 si falla fork.
 * ----------------------------------------------------------------------- */
int process_execute_fg(char **argv) {
    /* Validación defensiva de punteros */
    if (!argv || !argv[0]) {
        return -1;
    }

    /*
     * 0. VERIFICACIÓN PREVIA AL FORK (comportamiento de Bash/Zsh)
     * ---------------------------------------------------------------
     * Buscamos el ejecutable en $PATH ANTES de crear ningún proceso.
     * Si no existe, reportamos el error en el proceso padre y retornamos
     * sin haber llamado a fork(). De esta manera nunca se muestra un PID
     * para un comando que no existe.
     */
    if (!resolve_command_path(argv[0])) {
        fprintf(stderr, "miniOS: comando no encontrado: '%s'\n", argv[0]);
        return 127;
    }

    /* 1. FORK: Solo llegamos aquí si el binario existe y es ejecutable */
    pid_t pid = fork();

    if (pid < 0) {
        perror("miniOS: error al crear proceso hijo (fork)");
        return -1;
    }

    if (pid == 0) {
        /* =================================================================
         * PROCESO HIJO (pid == 0)
         * =================================================================
         * El binario ya fue validado por el padre, así que execvp() no
         * debería fallar por ENOENT. Si falla por otro motivo (permisos
         * revocados entre la validación y el exec, etc.), reporta y sale
         * con exit(126) — código estándar POSIX para "permiso denegado".
         */
        printf("[Proceso lanzado] PID: %d | PPID: %d (miniOS)\n\n", getpid(), getppid());
        fflush(stdout);

        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "miniOS: error al ejecutar '%s': %s\n", argv[0], strerror(errno));
            exit(126);
        }
    } else {
        /* =================================================================
         * PROCESO PADRE (pid > 0: PID del hijo)
         * =================================================================
         * Espera de forma bloqueante a que el proceso hijo termine.
         */
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            perror("miniOS: error en waitpid");
            return -1;
        }

        /* Salto visual para separar la salida del hijo del resumen del padre */
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
 * process_execute_bg — Ejecuta un comando en segundo plano (background).
 *
 * Issue 2.2 (SIS-43): Lanzamiento asíncrono con reserva de memoria simulada.
 * La interfaz memory_alloc / memory_free se conecta aquí; la implementación
 * real del subsistema de RAM llega en el Milestone 3.
 *
 * @param argv    Arreglo de argumentos terminado en NULL (argv[0]=comando).
 * @param mem_kb  Kilobytes de RAM simulada a reservar para este proceso.
 * @return PID del proceso hijo si se lanzó correctamente, -1 si falló.
 * ----------------------------------------------------------------------- */
int process_execute_bg(char **argv, int mem_kb) {
    (void)argv;
    (void)mem_kb;
    // Stub base — Se implementará en Issue 2.2 (SIS-43)
    return 0;
}

/* -----------------------------------------------------------------------
 * process_list_mini — Muestra la tabla de procesos en background (ps-mini).
 *
 * Issue 2.4 (SIS-45): Imprime PID, PPID, comando, RAM y estado de cada
 * proceso registrado en la tabla de background (bg_table).
 * ----------------------------------------------------------------------- */
void process_list_mini(void) {
    // Stub base — Se implementará en Issue 2.4 (SIS-45)
}
