#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell/shell.h"
#include "shell/parser.h"

void shell_init(void) {
    // Inicialización del estado del shell
}

int shell_dispatch(char *line) {
    char *argv[MAX_ARGS];
    int is_background = 0;
    int argc = parse_line(line, argv, &is_background);

    if (argc == 0) {
        return 0;
    }

    if (strcmp(argv[0], "exit") == 0) {
        printf("[INFO] Finalizando miniOS. ¡Hasta luego!\n");
        exit(0);
    }

    if (strcmp(argv[0], "help") == 0) {
        printf("miniOS — Comandos básicos disponibles:\n");
        printf("  help                Muestra esta ayuda\n");
        printf("  clear               Limpia la terminal\n");
        printf("  exit                Termina la sesión\n");
        return 0;
    }

    if (strcmp(argv[0], "clear") == 0) {
        printf("\033[H\033[J");
        return 0;
    }

    printf("[miniOS] Comando reconocido preliminarmente: %s (args: %d, bg: %d)\n",
           argv[0], argc, is_background);
    return 0;
}

void shell_loop(void) {
    char line[MAX_LINE];
    while (1) {
        printf("miniOS> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n[INFO] Saliendo del shell...\n");
            break; // EOF (Ctrl+D)
        }

        // Eliminar salto de línea (\r y \n)
        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) == 0) {
            continue;
        }

        shell_dispatch(line);
    }
}
