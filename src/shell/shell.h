#ifndef SHELL_H
#define SHELL_H

#define MAX_LINE 1024 // maxima cantidad de caracteres por linea
#define MAX_ARGS 64   // maxima cantidad de argumentos por linea

/**
 * @brief Inicializa el subsistema del shell.
 */
void shell_init(void);

/**
 * @brief Ciclo principal de lectura, evaluación e impresión (REPL).
 */
void shell_loop(void);

/**
 * @brief Enruta y despacha una línea de comandos ingresada.
 * @param line Línea de texto introducida por el usuario.
 * @return 0 en éxito, código de error en caso contrario.
 */
int shell_dispatch(char *line);

#endif // SHELL_H
