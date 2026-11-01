/*
 * parser.c — Issue 1.3 (SIS-40): Parser y Tokenizador de Línea de Comandos
 *
 * Implementa las dos variantes de tokenización:
 *  - parse_command(): versión con memoria dinámica (strdup), uso general.
 *  - parse_line():    versión liviana que opera sobre el string original.
 *  - free_command():  libera la memoria asignada por parse_command().
 *
 * Delimitadores reconocidos: espacio ' ', tabulación '\t'.
 * Operador de segundo plano: '&' como token final.
 */

/* Necesario para exponer strtok_r y strdup en glibc (Linux) y MinGW */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* -----------------------------------------------------------------------
 * parse_command — tokeniza 'line' en cmd->argv usando memoria propia
 * ----------------------------------------------------------------------- */
int parse_command(const char *line, Command *cmd) {
    if (!line || !cmd) return -1;

    /* Inicializar estructura en estado limpio */
    cmd->argc = 0;
    cmd->is_background = 0;
    for (int i = 0; i < MAX_ARGS; i++) {
        cmd->argv[i] = NULL;
    }

    /*
     * strtok_r modifica el string que recibe, así que trabajamos sobre
     * una copia local para no alterar la cadena original del llamador.
     */
    char copy[MAX_LINE];
    strncpy(copy, line, MAX_LINE - 1);
    copy[MAX_LINE - 1] = '\0'; // garantía de terminación segura

    char *saveptr = NULL;
    char *token = strtok_r(copy, " \t", &saveptr);

    while (token != NULL && cmd->argc < MAX_ARGS - 1) {
        /*
         * strdup() duplica el token en heap para que cmd->argv[i]
         * tenga vida propia más allá de esta función.
         * free_command() es quien debe liberar esa memoria.
         */
        cmd->argv[cmd->argc] = strdup(token);
        if (cmd->argv[cmd->argc] == NULL) {
            /* Fallo de asignación: liberar lo que se reservó y salir */
            fprintf(stderr, "[ERROR] parser: sin memoria al tokenizar\n");
            free_command(cmd);
            return -1;
        }
        cmd->argc++;
        token = strtok_r(NULL, " \t", &saveptr);
    }
    cmd->argv[cmd->argc] = NULL; // centinela final para execvp/execv

    /* Detectar operador de segundo plano '&' como último token */
    if (cmd->argc > 0 && strcmp(cmd->argv[cmd->argc - 1], "&") == 0) {
        cmd->is_background = 1;
        free(cmd->argv[cmd->argc - 1]); // liberar el token "&"
        cmd->argv[--cmd->argc] = NULL;  // reducir argc y poner centinela
    }

    return cmd->argc;
}

/* -----------------------------------------------------------------------
 * free_command — libera la memoria dinámica de una estructura Command
 * ----------------------------------------------------------------------- */
void free_command(Command *cmd) {
    if (!cmd) return;

    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
        cmd->argv[i] = NULL;
    }
    cmd->argc = 0;
    cmd->is_background = 0;
}

/* -----------------------------------------------------------------------
 * parse_line — tokenización liviana sobre el string original (sin strdup)
 *
 * ADVERTENCIA: modifica 'line' internamente (strtok_r inserta '\0').
 *              Los punteros en argv apuntan a regiones dentro de 'line';
 *              no se debe llamar a free() sobre ellos.
 * ----------------------------------------------------------------------- */
int parse_line(char *line, char **argv, int *is_background) {
    if (!line || !argv) return 0;

    if (is_background) *is_background = 0;

    int argc = 0;
    char *saveptr = NULL;
    char *token = strtok_r(line, " \t", &saveptr);

    while (token != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = token;
        token = strtok_r(NULL, " \t", &saveptr);
    }
    argv[argc] = NULL; // centinela final

    /* Detectar operador de segundo plano '&' como último token */
    if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
        if (is_background) *is_background = 1;
        argv[--argc] = NULL; // eliminar '&' del array y ajustar centinela
    }

    return argc;
}
