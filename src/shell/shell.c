/*
 * shell.c -- Issue 1.2 (SIS-39) + Issue 1.4 (SIS-41) + Issue 7.1 (SIS-60)
 *
 * Issue 1.2 (Aron): Ciclo principal REPL (shell_loop).
 * Issue 1.4 (JSON): Dispatcher central con tabla de built-ins y enrutamiento
 *                   modular a los comandos del sistema (shell_dispatch).
 * Issue 7.1 (JSON): Actualizacion de help con comandos del Modulo 2 (Hilos):
 *                   thread-demo [p] [c] [n] y thread-stats.
 */

#include "shell.h"
#include "parser.h"
#include "../process/process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Prototipos internos (funciones privadas de este archivo)
 * ----------------------------------------------------------------------- */
static int builtin_help(Command *cmd);
static int builtin_clear(Command *cmd);
static int builtin_exit(Command *cmd);

/* -----------------------------------------------------------------------
 * Tabla de despacho de comandos built-in
 *
 * Cada entrada asocia el nombre del comando con su funcion manejadora.
 * Para agregar un nuevo built-in solo hay que anhadir una fila aqui,
 * sin tocar ningun otro bloque de codigo (principio Open/Closed).
 * ----------------------------------------------------------------------- */
typedef struct {
    const char *name;
    int (*handler)(Command *cmd);
} BuiltinEntry;

static const BuiltinEntry BUILTINS[] = {
    { "help",  builtin_help  },
    { "clear", builtin_clear },
    { "exit",  builtin_exit  },
};

/* Numero de entradas en la tabla, calculado en tiempo de compilacion */
#define BUILTIN_COUNT ((int)(sizeof(BUILTINS) / sizeof(BUILTINS[0])))

/* -----------------------------------------------------------------------
 * shell_init -- Inicializacion del subsistema del shell
 *
 * Por ahora vacia; en issues posteriores inicializara la tabla de
 * procesos en background, la configuracion de RAM, etc.
 * ----------------------------------------------------------------------- */
void shell_init(void) {
    /* Issue 1.1: stub minimo -- se expandira en M3 (Modulo de Memoria) */
}

/* -----------------------------------------------------------------------
 * Implementaciones de los comandos built-in
 * ----------------------------------------------------------------------- */

/**
 * help -- Lista todos los comandos disponibles agrupados por modulo
 * o muestra la ayuda especifica si se especifica un comando.
 *
 * Actualizado en Issue 7.1: se agregaron thread-demo y thread-stats
 * del Modulo 2 (Hilos) con sus descripciones completas.
 */
static int builtin_help(Command *cmd) {
    /* Si no se especifico un argumento, mostrar la ayuda general */
    if (cmd->argc <= 1) {
        printf("\n");
        printf("  miniOS -- Shell Interactivo Educativo en C11\n");
        printf("  ============================================\n\n");
        printf("  Uso: help [comando]\n\n");

        printf("  [Shell / Built-ins]\n");
        printf("    help              Muestra este mensaje de ayuda o el detalle de un comando.\n");
        printf("    clear             Limpia la pantalla de la terminal.\n");
        printf("    exit              Finaliza la sesion del shell.\n");
        printf("\n");

        printf("  [Modulo 1 -- Procesos]  (disponible en Issue 2.1+)\n");
        printf("    <comando> [args]  Ejecuta un comando del sistema en primer plano.\n");
        printf("    <comando> [args] & Ejecuta un comando en segundo plano.\n");
        printf("    ps-mini           Lista los procesos activos de la sesion.\n");
        printf("\n");

        /* --- Issue 7.1: Modulo 2 actualizado con thread-demo y thread-stats --- */
        printf("  [Modulo 2 -- Hilos]     (disponible en Issue 7.1+)\n");
        printf("    thread-demo [p] [c] [n]\n");
        printf("                      Lanza la demo Productor-Consumidor con p productores,\n");
        printf("                      c consumidores y n items por productor.\n");
        printf("                      Valores por defecto: 2 productores, 2 consumidores, 10 items.\n");
        printf("    thread-stats      Muestra estadisticas de la ultima sesion de hilos:\n");
        printf("                      items producidos, consumidos, perdidas y tiempo de ejecucion.\n");
        printf("\n");

        printf("  [Modulo 3 -- Memoria]   (disponible en Issue 3.1+)\n");
        printf("    mem-config        Configura el tamanho de la RAM simulada.\n");
        printf("    mem-view          Muestra el estado actual de la memoria.\n");
        printf("\n");
        return 0;
    }

    /* Ayuda detallada para un comando especifico */
    const char *topic = cmd->argv[1];

    if (strcmp(topic, "help") == 0) {
        printf("\n  help [comando]\n");
        printf("    Muestra informacion de ayuda general o el detalle de un comando especifico.\n\n");
    } else if (strcmp(topic, "clear") == 0) {
        printf("\n  clear\n");
        printf("    Limpia el contenido de la pantalla de la terminal mediante secuencias ANSI.\n\n");
    } else if (strcmp(topic, "exit") == 0) {
        printf("\n  exit [codigo]\n");
        printf("    Finaliza la sesion de miniOS. Opcionalmente acepta un codigo numerico de salida.\n\n");
    } else if (strcmp(topic, "ps-mini") == 0) {
        printf("\n  ps-mini\n");
        printf("    Muestra la lista de procesos activos y en segundo plano gestionados por el shell.\n\n");
    } else if (strcmp(topic, "thread-demo") == 0) {
        /* Actualizado en Issue 7.1 con sintaxis completa y descripcion de parametros */
        printf("\n  thread-demo [p] [c] [n]\n");
        printf("    Ejecuta la simulacion concurrente del problema clasico Productor-Consumidor.\n\n");
        printf("    Parametros opcionales (todos deben ser enteros positivos >= 2):\n");
        printf("      p  Numero de hilos productores  (defecto: 2)\n");
        printf("      c  Numero de hilos consumidores (defecto: 2)\n");
        printf("      n  Items que genera cada productor (defecto: 10)\n\n");
        printf("    Ejemplo:\n");
        printf("      thread-demo 3 2 20   # 3 productores, 2 consumidores, 20 items c/u\n");
        printf("      thread-demo          # ejecuta con valores por defecto\n\n");
        printf("    Usa un bufer circular de %d slots protegido con pthread_mutex_t y\n", 8);
        printf("    pthread_cond_t para garantizar 0 perdidas de datos.\n\n");
    } else if (strcmp(topic, "thread-stats") == 0) {
        /* Nuevo en Issue 7.1 */
        printf("\n  thread-stats\n");
        printf("    Muestra las estadisticas de la ultima sesion de hilos ejecutada con thread-demo.\n\n");
        printf("    Incluye: productores, consumidores, items producidos, items consumidos,\n");
        printf("    perdida de datos (siempre debe ser 0) y tiempo total de computo.\n\n");
        printf("    Debe ejecutarse despues de thread-demo; si no hay sesion previa muestra aviso.\n\n");
    } else if (strcmp(topic, "mem-config") == 0) {
        printf("\n  mem-config <tamanho_kb>\n");
        printf("    Configura el tamanho de la memoria RAM fisica simulada en kilobytes.\n\n");
    } else if (strcmp(topic, "mem-view") == 0) {
        printf("\n  mem-view\n");
        printf("    Visualiza el mapa de asignacion de marcos/paginas y fragmentacion de memoria.\n\n");
    } else {
        fprintf(stderr, "miniOS: help: no hay entrada de ayuda para '%s'. Usa 'help' para ver la lista.\n", topic);
        return 1;
    }

    return 0;
}

/**
 * clear -- Limpia la pantalla usando secuencias de escape ANSI.
 */
static int builtin_clear(Command *cmd) {
    (void)cmd;
    printf("\033[H\033[J");
    fflush(stdout);
    return 0;
}

/**
 * exit -- Termina la sesion del shell limpiamente.
 *
 * Devuelve un codigo especial (-1) que shell_dispatch reenvía a
 * shell_loop para que sepa que debe romper el bucle REPL.
 * Soporta un codigo numerico opcional (ej: exit 0). Si recibe texto
 * o argumentos invalidos, emite un error y no finaliza la sesion.
 */
static int builtin_exit(Command *cmd) {
    if (cmd->argc > 2) {
        fprintf(stderr, "miniOS: exit: demasiados argumentos\n");
        return 1;
    }

    if (cmd->argc == 2) {
        char *endptr = NULL;
        long code = strtol(cmd->argv[1], &endptr, 10);
        if (*endptr != '\0' || endptr == cmd->argv[1]) {
            fprintf(stderr, "miniOS: exit: '%s': se requiere un argumento numerico\n", cmd->argv[1]);
            return 1;
        }
        printf("[INFO] Finalizando miniOS con codigo %ld. Hasta luego!\n", code);
        return -1;
    }

    printf("[INFO] Finalizando miniOS. Hasta luego!\n");
    return -1; /* senal de salida para shell_loop */
}

/* -----------------------------------------------------------------------
 * shell_dispatch -- Enrutador central de comandos (Issue 1.4 / SIS-41)
 *
 * Flujo:
 *  1. Tokeniza 'line' usando parse_command() del parser (Issue 1.3).
 *  2. Si la linea estaba vacia, retorna 0 sin hacer nada.
 *  3. Busca argv[0] en la tabla de built-ins y lo ejecuta si lo encuentra.
 *  4. Si no es un built-in, llama a process_execute_fg (Issue 2.1).
 *
 * @param line  Linea de texto cruda leida por shell_loop (se trabaja sobre copia).
 * @return 0 en exito, -1 si el shell debe terminar (comando exit).
 * ----------------------------------------------------------------------- */
int shell_dispatch(char *line) {
    Command cmd;
    int argc = parse_command(line, &cmd);

    /* Linea vacia o solo espacios: no hacer nada */
    if (argc == 0) {
        free_command(&cmd);
        return 0;
    }

    /* Buscar argv[0] en la tabla de built-ins */
    for (int i = 0; i < BUILTIN_COUNT; i++) {
        if (strcmp(cmd.argv[0], BUILTINS[i].name) == 0) {
            int result = BUILTINS[i].handler(&cmd);
            free_command(&cmd);
            return result;
        }
    }

    /*
     * Comando no reconocido como built-in.
     * Issue 2.1 (Modulo 1: Procesos): Ejecucion externa sincrona en foreground.
     * (En Issue 2.2 se evaluara cmd.is_background para bifurcar a process_execute_bg).
     */
    int result = process_execute_fg(cmd.argv);
    free_command(&cmd);
    return result;
}

/* -----------------------------------------------------------------------
 * shell_loop -- Ciclo principal REPL (Issue 1.2 / SIS-39)
 *
 * Lee lineas del usuario, las limpia y las envia a shell_dispatch.
 * Si dispatch devuelve -1 (senal de exit), rompe el bucle.
 * ----------------------------------------------------------------------- */
void shell_loop(void) {
    char line[MAX_LINE];

    while (1) {
        printf("miniOS> ");
        fflush(stdout);

        /* Detecta Ctrl+D (EOF) */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n[INFO] Forzando cierre, Saliendo del shell...\n");
            break;
        }

        /* Elimina '\n' o '\r\n' al final de la linea leida */
        line[strcspn(line, "\r\n")] = '\0';

        /* Ignora lineas completamente vacias (solo Enter) */
        if (strlen(line) == 0) continue;

        /* Despacha el comando; si retorna -1 es senal de exit */
        if (shell_dispatch(line) == -1) break;
    }
}
