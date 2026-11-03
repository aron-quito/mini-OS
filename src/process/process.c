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

/**
 * process_execute_fg — Ejecuta un comando externo en primer plano (foreground).
 *
 * Implementa el patrón clásico de UNIX para ejecución síncrona (Issue 2.1 / SIS-42):
 *  1. fork(): Duplica el proceso padre (shell) creando un clon hijo idéntico.
 *  2. execvp() [en el hijo]: Busca el binario en las rutas de $PATH y reemplaza
 *     la imagen del proceso hijo por el nuevo programa pasándole argv completo.
 *     Si execvp falla (ej. comando inexistente), reporta el error y sale con exit(127).
 *  3. waitpid() [en el padre]: Bloquea al shell padre hasta que el hijo finalice,
 *     capturando su código de salida (exit status) o señal de terminación.
 *
 * @param argv Arreglo de cadenas terminado en NULL (argv[0]=comando, argv[1..n]=argumentos).
 * @return Código de retorno del proceso hijo (0 = éxito, >0 = error), o -1 si falla fork/waitpid.
 */
int process_execute_fg(char **argv) {
    /* Validación defensiva de punteros */
    if (!argv || !argv[0]) {
        return -1;
    }

    /* 1. Crear el proceso hijo */
    pid_t pid = fork();

    if (pid < 0) {
        /* Error en la llamada al sistema fork() */
        perror("miniOS: error al crear proceso hijo (fork)");
        return -1;
    }

    if (pid == 0) {
        /* =================================================================
         * PROCESO HIJO (pid == 0)
         * =================================================================
         * Informa su creación mostrando su PID y el PPID de miniOS.
         * Se deja un salto de línea para diferenciarlo de la salida del comando.
         */
        printf("[Proceso lanzado] PID: %d | PPID: %d (miniOS)\n\n", getpid(), getppid());
        fflush(stdout);

        if (execvp(argv[0], argv) < 0) {
            if (errno == ENOENT) {
                fprintf(stderr, "miniOS: comando no encontrado: '%s'\n", argv[0]);
            } else {
                fprintf(stderr, "miniOS: error al ejecutar '%s': %s\n", argv[0], strerror(errno));
            }
            /* Terminar inmediatamente el hijo para evitar ejecutar el bucle del shell */
            exit(127);
        }
    } else {
        /* =================================================================
         * PROCESO PADRE (pid > 0: PID del hijo)
         * =================================================================
         * Espera de forma bloqueante a que el proceso hijo termine su ejecución.
         */
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            perror("miniOS: error en waitpid");
            return -1;
        }

        /* Salto visual corto para separar la salida del hijo del informe final del padre */
        printf("\n");

        /* Inspeccionar cómo terminó el hijo */
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

int process_execute_bg(char **argv, int mem_kb) {
    (void)argv;
    (void)mem_kb;
    return 0;
}

void process_list_mini(void) {
    // Stub base — Se implementará en Issue 2.4 (SIS-45)
}
