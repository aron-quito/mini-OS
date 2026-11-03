#ifndef PARSER_H
#define PARSER_H

#include "shell.h" // MAX_LINE, MAX_ARGS

/**
 * @brief Estructura que representa un comando ya tokenizado.
 *
 * Contiene el arreglo de argumentos (argv), el conteo de argumentos (argc)
 * y la bandera de ejecución en segundo plano (is_background).
 * La memoria de cada argv[i] es dinámica — usar free_command() para liberar.
 */
typedef struct {
    char *argv[MAX_ARGS]; // Argumentos del comando (último elemento siempre NULL)
    int argc;             // Cantidad de argumentos detectados
    int is_background;    // 1 si el comando termina con '&', 0 si no
} Command;

/**
 * @brief Tokeniza una línea de comando en una estructura Command.
 *
 * Separa la línea por espacios y tabulaciones. Si el último token es '&',
 * activa cmd->is_background = 1 y lo elimina de argv. Cada token se
 * almacena con memoria propia (strdup), por lo que se debe llamar a
 * free_command() cuando ya no se necesite.
 *
 * @param line Cadena de entrada (no es modificada, se trabaja sobre una copia).
 * @param cmd  Puntero a la estructura Command a rellenar.
 * @return Cantidad de argumentos (argc), o -1 en caso de error de memoria.
 */
int parse_command(const char *line, Command *cmd);

/**
 * @brief Libera toda la memoria dinámica asignada por parse_command.
 *
 * Recorre cmd->argv, llama a free() en cada elemento y resetea
 * argc e is_background a 0.
 *
 * @param cmd Puntero a la estructura Command a liberar.
 */
void free_command(Command *cmd);

/**
 * @brief Tokeniza una línea directamente en un arreglo argv existente.
 *
 * Variante liviana de parse_command: opera sobre la cadena original
 * (la modifica internamente con strtok_r) y guarda punteros directos
 * sin duplicar memoria. No requiere free_command.
 *
 * @param line          Cadena original a procesar (es modificada).
 * @param argv          Arreglo de punteros donde se almacenarán los tokens.
 * @param is_background Puntero a entero: 1 si termina en '&', 0 si no.
 * @return Cantidad de argumentos (argc).
 */
int parse_line(char *line, char **argv, int *is_background);

#endif // PARSER_H
