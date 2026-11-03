/*
 * shell.c — Issue 1.2 (SIS-39) + Issue 1.4 (SIS-41) + Issue 2.2 (SIS-43)
 *
 * Issue 1.2 (Aron):  Ciclo principal REPL (shell_loop).
 * Issue 1.4 (JSON):  Dispatcher central con tabla de built-ins.
 * Issue 2.2 (Aron):  Ejecución asíncrona en background con operador '&':
 *                    extracción del argumento de memoria (KB), bifurcación
 *                    fg/bg en shell_dispatch y cosecha de procesos terminados
 *                    antes de cada prompt para evitar solapamientos.
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
    /* Inicializar subsistemas del shell */
    process_init(); /* Inicializa la tabla de control de procesos en background (Issue 2.2 / 2.3) */
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
        printf("    help                    Muestra este mensaje de ayuda o el detalle de un comando.\n");
        printf("    clear                   Limpia la pantalla de la terminal.\n");
        printf("    exit                    Finaliza la sesión del shell.\n");
        printf("\n");
        printf("  [Módulo 1 — Procesos]  (Issue 2.1 / 2.2)\n");
        printf("    <cmd> [args]            Ejecuta un comando del sistema en primer plano.\n");
        printf("    <cmd> [args] [kb] &     Ejecuta en segundo plano con reserva de RAM simulada.\n");
        printf("    sleep <seg> [kb] &      Lanza una pausa asíncrona simulando proceso en background.\n");
        printf("    ps-mini                 Lista los procesos activos de la sesión.\n");
        printf("\n");
        printf("  [Módulo 2 — Hilos]     (disponible en Issue 7.1+)\n");
        printf("    thread-demo             Inicia la demo del productor-consumidor.\n");
        printf("\n");
        printf("  [Módulo 3 — Memoria]   (disponible en Issue 3.1+)\n");
        printf("    mem-config              Configura el tamaño de la RAM simulada.\n");
        printf("    mem-view                Muestra el estado actual de la memoria.\n");
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
    } else if (strcmp(topic, "sleep") == 0) {
        printf("\n  sleep <segundos> [mem_kb] &\n");
        printf("    Ejecuta un proceso de espera en segundo plano simulando carga y memoria (Issue 2.2).\n");
        printf("    Parámetros:\n");
        printf("      <segundos>  Tiempo que el proceso permanecerá activo.\n");
        printf("      [mem_kb]    (Opcional) Kilobytes de RAM simulada a reservar (por defecto: 128 KB).\n");
        printf("      &           Obligatorio para ejecución asíncrona en background.\n");
        printf("    Ejemplos:\n");
        printf("      sleep 5 &\n");
        printf("      sleep 10 256 &\n\n");
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
 * extract_mem_kb — Extrae el argumento de memoria del arreglo de argumentos.
 *
 * Cuando el usuario ejecuta un comando en background con tamaño explícito:
 *   sleep 10 256 &   →   argv = ["sleep", "10", "256", NULL], is_background=1
 *
 * El último argumento numérico ("256") es el tamaño de RAM en KB que pide el
 * proceso. No debe pasarse a execvp() ya que no es un argumento de 'sleep'.
 * Esta función lo detecta, lo extrae del arreglo argv y ajusta argc.
 *
 * Si el último argumento no es un número puro, se usa DEFAULT_BG_MEM_KB (128 KB).
 *
 * @param argv      Arreglo de argumentos terminado en NULL (modificado in-place).
 * @param argc_ptr  Puntero a argc, decrementado si se extrajo el argumento.
 * @return Kilobytes solicitados (extraído o por defecto).
 * ----------------------------------------------------------------------- */
static int extract_mem_kb(char **argv, int *argc_ptr) {
    int argc = *argc_ptr;

    /*
     * Se necesita al menos: argv[0]=comando, argv[1]=argumento, argv[2]=KB
     * Con argc < 3 no hay forma de distinguir el único argumento numérico
     * (que pertenece al comando) del argumento de KB de RAM.
     * Ejemplo:
     *   "sleep 3 &"       → argc=2, argv=["sleep","3"] → KB por defecto, sleep recibe "3"
     *   "sleep 10 256 &"  → argc=3, argv=["sleep","10","256"] → extrae 256 como KB
     */
    if (argc < 3) {
        return DEFAULT_BG_MEM_KB;
    }

    /* Verificar si el último argumento es un entero puro positivo */
    char *last = argv[argc - 1];
    char *endptr = NULL;
    long val = strtol(last, &endptr, 10);

    if (*endptr == '\0' && endptr != last && val > 0) {
        /* Es un número: extraerlo del argv y ajustar argc */
        argv[argc - 1] = NULL;
        (*argc_ptr)--;
        return (int)val;
    }

    return DEFAULT_BG_MEM_KB; /* Sin argumento numérico: usar valor por defecto */
}

/* -----------------------------------------------------------------------
 * shell_dispatch — Enrutador central de comandos (Issues 1.4 + 2.2)
 *
 * Flujo:
 *  1. Tokeniza 'line' usando parse_command() (Issue 1.3).
 *  2. Si la línea estaba vacía, retorna 0.
 *  3. Busca argv[0] en la tabla de built-ins y lo ejecuta si lo encuentra.
 *  4. Si no es un built-in y cmd.is_background == 0:
 *       → process_execute_fg() — síncrono (Issue 2.1).
 *  5. Si no es un built-in y cmd.is_background == 1:
 *       → Extrae el argumento de KB con extract_mem_kb().
 *       → process_execute_bg() — asíncrono con reserva de RAM (Issue 2.2).
 *
 * @param line  Línea de texto cruda leída por shell_loop.
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

    int result;

    if (cmd.is_background) {
        /*
         * Issue 2.2 — Ejecución en background con operador '&':
         * Extraer el argumento de KB antes de llamar a execvp para que el
         * programa externo no lo reciba como argumento desconocido.
         * Ejemplo: "sleep 10 256 &" → execvp("sleep", ["sleep", "10", NULL])
         *                              mem_kb = 256
         */
        int mem_kb = extract_mem_kb(cmd.argv, &cmd.argc);
        result = process_execute_bg(cmd.argv, mem_kb);
    } else {
        /* Issue 2.1 — Ejecución en foreground síncrona */
        result = process_execute_fg(cmd.argv);
    }

    free_command(&cmd);
    return result;
}

/* -----------------------------------------------------------------------
 * shell_loop — Ciclo principal REPL (Issue 1.2 + Issue 2.2)
 *
 * Lee líneas del usuario, las limpia y las envía a shell_dispatch.
 * Antes de mostrar cada prompt, llama a process_reap_bg() para cosechar
 * procesos en background que hayan terminado. Esto garantiza que las
 * notificaciones de fin aparezcan en una línea limpia y nunca se solapen
 * con el texto que el usuario está escribiendo.
 *
 * Si dispatch devuelve -1 (señal de exit), rompe el bucle.
 * ----------------------------------------------------------------------- */
void shell_loop(void) {
    char line[MAX_LINE];

    while (1) {
        /*
         * Cosechar procesos en background terminados ANTES del prompt.
         * Garantiza que la notificación salga en una línea limpia y no
         * se mezcle con la entrada del usuario.
         */
        process_reap_bg();

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