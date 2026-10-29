#include "shell.h"
#include <stdio.h>

void shell_init(void) {
  // Inicialización base del shell
  // objetivo: inicializar las memorias
  // resetear las banderas de la shell, etc.
}

void shell_loop(void) {
  // Stub base de Issue 1.1 — Se implementará en detalle en la Issue 1.2
  // (SIS-39)
  printf("miniOS> \n");
}

int shell_dispatch(char *line) {
  (void)line;
  // Stub base — Se implementará en la Issue 1.4 (SIS-41)
  return 0;
}
