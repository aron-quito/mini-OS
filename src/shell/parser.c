#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "shell.h"

int parse_line(char *line, char **argv, int *is_background) {
    int argc = 0;
    *is_background = 0;

    char *saveptr = NULL;
    char *token = strtok_r(line, " \t\r\n", &saveptr);

    while (token != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = token;
        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }
    argv[argc] = NULL;

    if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
        *is_background = 1;
        argv[argc - 1] = NULL;
        argc--;
    }

    return argc;
}
