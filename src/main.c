#include <stdio.h>
#include <stdlib.h>
#include "shell/shell.h"
#include "process/process.h"
#include "process/signals.h"
#include "memory/memory.h"

static void print_banner(void) {
    printf("================================================================\n");
    printf("         miniOS — Shell Interactivo Educativo en C11            \n");
    printf("  [Módulos: Procesos (M1) | Hilos (M2) | Memoria RAM (M3)]     \n");
    printf("  Escriba 'help' para ver la lista de comandos o 'exit' para salir.\n");
    printf("================================================================\n\n");
}

int main(void) {
    print_banner();

    // 1. Inicialización de subsistemas modulares
    signals_init();
    process_init();
    memory_init(DEFAULT_RAM_KB, MEM_ALGO_FIRST_FIT);
    shell_init();

    // 2. Iniciar el bucle interactivo del shell (REPL)
    shell_loop();

    // 3. Limpieza de recursos al salir
    memory_cleanup();

    return 0;
}
