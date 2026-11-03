/*
 * shell.c — Issue 1.2 (SIS-39) + Issue 1.4 (SIS-41)
 *
 * Issue 1.2 (Aron): Ciclo principal REPL (shell_loop).
 * Issue 1.4 (JSON): Dispatcher central con tabla de built-ins y enrutamiento
 *                   modular a los comandos del sistema (shell_dispatch).
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
 * Cada entrada asocia el nombre del comando con su función manejadora.
 * Para agregar un nuevo built-in solo hay que añadir una fila aquí,
 * sin tocar ningún otro bloque de código (principio Open/Closed).
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

/* Número de entradas en la tabla, calculado en tiempo de compilación */
#define BUILTIN_COUNT ((int)(sizeof(BUILTINS) / sizeof(BUILTINS[0])))

/* -----------------------------------------------------------------------
 * shell_init — Inicialización del subsistema del shell
 *
 * Por ahora vacía; en issues posteriores inicializará la tabla de
 * procesos en background, la configuración de RAM, etc.
 * ----------------------------------------------------------------------- */
void shell_init(void) {
    /* Issue 1.1: stub mínimo — se expandirá en M3 (Módulo de Memoria) */
}

/* -----------------------------------------------------------------------
 * Implementaciones de los comandos built-in
 * ----------------------------------------------------------------------- */

/**
 * help — Lista todos los comandos disponibles agrupados por módulo
 * o muestra la ayuda específica si se especifica un comando.
 */
static int builtin_help(Command *cmd) {
    /* Si no se especificó un argumento, mostrar la ayuda general */
    if (cmd->argc <= 1) {
        printf("\n");
        printf("  miniOS — Shell Interactivo Educativo en C11\n");
        printf("  ============================================\n\n");
        printf("  Uso: help [comando]\n\n");
        printf("  [Shell / Built-ins]\n");
        printf("    help              Muestra este mensaje de ayuda o el detalle de un comando.\n");
        printf("    clear             Limpia la pantalla de la terminal.\n");
        printf("    exit              Finaliza la sesión del shell.\n");
        printf("\n");
        printf("  [Módulo 1 — Procesos]  (disponible en Issue 2.1+)\n");
        printf("    <comando> [args]  Ejecuta un comando del sistema en primer plano.\n");
        printf("    <comando> [args] & Ejecuta un comando en segundo plano.\n");
        printf("    ps-mini           Lista los procesos activos de la sesión.\n");
        printf("\n");
        printf("  [Módulo 2 — Hilos]     (disponible en Issue 7.1+)\n");
        printf("    thread-demo       Inicia la demo del productor-consumidor.\n");
        printf("\n");
        printf("  [Módulo 3 — Memoria]   (disponible en Issue 3.1+)\n");
        printf("    mem-config        Configura el tamaño de la RAM simulada.\n");
        printf("    mem-view          Muestra el estado actual de la memoria.\n");
        printf("\n");
        return 0;
    }

    /* Ayuda detallada para un comando específico */
    const char *topic = cmd->argv[1];

    if (strcmp(topic, "help") == 0) {
        printf("\n  help [comando]\n");
        printf("    Muestra información de ayuda general o el detalle de un comando específico.\n\n");
    } else if (strcmp(topic, "clear") == 0) {
        printf("\n  clear\n");
        printf("    Limpia el contenido de la pantalla de la terminal mediante secuencias ANSI.\n\n");
    } else if (strcmp(topic, "exit") == 0) {
        printf("\n  exit [código]\n");
        printf("    Finaliza la sesión de miniOS. Opcionalmente acepta un código numérico de salida.\n\n");
    } else if (strcmp(topic, "ps-mini") == 0) {
        printf("\n  ps-mini\n");
        printf("    Muestra la lista de procesos activos y en segundo plano gestionados por el shell.\n\n");
    } else if (strcmp(topic, "thread-demo") == 0) {
        printf("\n  thread-demo\n");
        printf("    Ejecuta la simulación interactiva del problema clásico Productor-Consumidor.\n\n");
    } else if (strcmp(topic, "mem-config") == 0) {
        printf("\n  mem-config <tamaño_kb>\n");
        printf("    Configura el tamaño de la memoria RAM física simulada en kilobytes.\n\n");
    } else if (strcmp(topic, "mem-view") == 0) {
        printf("\n  mem-view\n");
        printf("    Visualiza el mapa de asignación de marcos/páginas y fragmentación de memoria.\n\n");
    } else {
        fprintf(stderr, "miniOS: help: no hay entrada de ayuda para '%s'. Usa 'help' para ver la lista.\n", topic);
        return 1;
    }

    return 0;
}

/**
 * clear — Limpia la pantalla usando secuencias de escape ANSI.
 */
static int builtin_clear(Command *cmd) {
    (void)cmd;
    printf("\033[H\033[J");
    fflush(stdout);
    return 0;
}

/**
 * exit — Termina la sesión del shell limpiamente.
 *
 * Devuelve un código especial (-1) que shell_dispatch reenvía a
 * shell_loop para que sepa que debe romper el bucle REPL.
 * Soporta un código numérico opcional (ej: exit 0). Si recibe texto
 * o argumentos inválidos, emite un error y no finaliza la sesión.
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
            fprintf(stderr, "miniOS: exit: '%s': se requiere un argumento numérico\n", cmd->argv[1]);
            return 1;
        }
        printf("[INFO] Finalizando miniOS con código %ld. ¡Hasta luego!\n", code);
        return -1;
    }

    printf("[INFO] Finalizando miniOS. ¡Hasta luego!\n");
    return -1; /* señal de salida para shell_loop */
}

/* -----------------------------------------------------------------------
 * shell_dispatch — Enrutador central de comandos (Issue 1.4 / SIS-41)
 *
 * Flujo:
 *  1. Tokeniza 'line' usando parse_command() del parser (Issue 1.3).
 *  2. Si la línea estaba vacía, retorna 0 sin hacer nada.
 *  3. Busca argv[0] en la tabla de built-ins y lo ejecuta si lo encuentra.
 *  4. Si no es un built-in, emite el error estándar de comando no encontrado.
 *     (En Issue 2.1 se sustituirá este mensaje por fork() + execvp()).
 *
 * @param line  Línea de texto cruda leída por shell_loop (se trabaja sobre copia).
 * @return 0 en éxito, -1 si el shell debe terminar (comando exit).
 * ----------------------------------------------------------------------- */
int shell_dispatch(char *line) {
    Command cmd;
    int argc = parse_command(line, &cmd);

    /* Línea vacía o solo espacios: no hacer nada */
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
     * Issue 2.1 (Módulo 1: Procesos): Ejecución externa síncrona en foreground.
     * (En Issue 2.2 se evaluará cmd.is_background para bifurcar a process_execute_bg).
     */
    int result = process_execute_fg(cmd.argv);
    free_command(&cmd);
    return result;
}

/* -----------------------------------------------------------------------
 * shell_loop — Ciclo principal REPL (Issue 1.2 / SIS-39)
 *
 * Lee líneas del usuario, las limpia y las envía a shell_dispatch.
 * Si dispatch devuelve -1 (señal de exit), rompe el bucle.
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

        /* Elimina '\n' o '\r\n' al final de la línea leída */
        line[strcspn(line, "\r\n")] = '\0';

        /* Ignora líneas completamente vacías (solo Enter) */
        if (strlen(line) == 0) continue;

        /* Despacha el comando; si retorna -1 es señal de exit */
        if (shell_dispatch(line) == -1) break;
    }
}