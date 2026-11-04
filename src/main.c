/**
 * @file main.c
 * @brief Punto de entrada principal de miniOS.
 *
 * Flujo de ejecución:
 * 1. Muestra el banner inicial del sistema.
 * 2. Llama a shell_init(): inicializa los subsistemas del kernel (en Issue 3.1
 *    configura e inicializa la memoria física simulada con 128 KB en First-Fit).
 * 3. Inicia el ciclo interactivo REPL con shell_loop().
 */

#include <stdio.h>
#include "shell/shell.h"

int main(void) {
    printf("================================================================\n");
    printf("         miniOS — Shell Interactivo Educativo en C11            \n");
    printf("================================================================\n\n");

    // Inicializa subsistemas del sistema (memoria, procesos, background, etc.)
    shell_init();

    // Ciclo interactivo Read-Eval-Print-Loop (REPL)
    shell_loop();

    return 0;
}
