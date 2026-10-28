#ifndef PARSER_H
#define PARSER_H

/**
 * @brief Tokeniza una línea de comando en un arreglo argv y detecta si es background (&).
 * @param line Cadena original a procesar.
 * @param argv Arreglo de punteros donde se almacenarán los tokens (terminado en NULL).
 * @param is_background Puntero a entero donde se colocará 1 si termina en '&' o 0 si no.
 * @return Cantidad de argumentos (argc).
 */
int parse_line(char *line, char **argv, int *is_background);

#endif // PARSER_H
