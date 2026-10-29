#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void shell_init(void) {
  // Inicialización base del shell
  // objetivo: inicializar las memorias
  // resetear las banderas de la shell, etc.
}

void shell_loop(void) {
  // Stub base de Issue 1.1 — Se implementará en detalle en la Issue 1.2
  // (SIS-39)
  char line[MAX_LINE];
  while (1) {
    printf("miniOS> ");
    fflush(stdout);

    // Detecta Ctrl+D (EOF)
    if (fgets(line, sizeof(line), stdin) == NULL) {
      printf("\n[INFO] Forzando cierre, Saliendo del shell...\n");
      break;
    }

    // Transforma "exit\n" en "exit", "ls -l\n" en "ls -l"
    line[strcspn(line, "\r\n")] = '\0';

    // Detecta salida por comando "exit"
    if (strcmp(line, "exit") == 0) {
      printf("[INFO] Finalizando miniOS. ¡Hasta luego!\n");
      break;
    }

    // Detecta entrada vacía
    if (strlen(line) <= 1)
      continue; // Enter vacío
    shell_dispatch(line);
  }
}

int shell_dispatch(char *line) {
  (void)line;
  // Stub base — Se implementará en la Issue 1.4 (SIS-41)
  return 0;
}
