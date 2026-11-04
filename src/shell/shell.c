/*
 * shell.c -- Issue 1.2 (SIS-39) + Issue 1.4 (SIS-41) + Issue 7.1 (SIS-60)
 *           + Issue 3.1 (SIS-46) + Issue 7.3 (SIS-62)
 *
 * Issue 1.2 (Aron): Ciclo principal REPL (shell_loop).
 * Issue 1.4 (JSON): Dispatcher central con tabla de built-ins y enrutamiento
 *                   modular a los comandos del sistema (shell_dispatch).
 * Issue 3.1 (Luis): Inicializacion del subsistema de memoria fisica simulada.
 * Issue 7.1 (JSON): Actualizacion de help con comandos del Modulo 2 (Hilos).
 * Issue 7.3 (JSON): Comando thread-demo [p] [c] [n] conectado a
 *                   thread_demo_run() del modulo de hilos (producer_consumer.c).
 */

#include "shell.h"
#include "parser.h"
#include "../process/process.h"
#include "../memory/memory.h"
#include "../threads/producer_consumer.h"  /* thread_demo_run, thread_get_stats -- Issue 7.3 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Prototipos internos (funciones privadas de este archivo)
 * ----------------------------------------------------------------------- */
static int builtin_help(Command *cmd);
static int builtin_clear(Command *cmd);
static int builtin_exit(Command *cmd);
static int builtin_thread_demo(Command *cmd);   /* Issue 7.3 (SIS-62) */

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
    { "help",        builtin_help        },
    { "clear",       builtin_clear       },
    { "exit",        builtin_exit        },
    { "thread-demo", builtin_thread_demo }, /* Issue 7.3 (SIS-62) */
};

/* Numero de entradas en la tabla, calculado en tiempo de compilacion */
#define BUILTIN_COUNT ((int)(sizeof(BUILTINS) / sizeof(BUILTINS[0])))

/* -----------------------------------------------------------------------
 * shell_init -- Inicializacion de los subsistemas del sistema
 *
 * Issue 3.1 (SIS-46): Inicializa la memoria fisica simulada con 128 KB
 * bajo el esquema de particionamiento dinamico First-Fit.
 * ----------------------------------------------------------------------- */
void shell_init(void) {
    /* Inicializar subsistema de memoria para la Issue 3.1 (SIS-46) */
    memory_init(128, MEM_ALGO_FIRST_FIT);

    if (mem_state.block_list != NULL) {
        printf("[miniOS Kernel] Memoria inicializada: %d KB RAM | Bloque inicial: %d-%d KB (%s)\n\n",
               mem_state.total_ram_kb,
               mem_state.block_list->start_kb,
               mem_state.block_list->size_kb,
               mem_state.block_list->status == BLOCK_FREE ? "BLOCK_FREE" : "BLOCK_USED");
    }
}

/* -----------------------------------------------------------------------
 * Implementaciones de los comandos built-in
 * ----------------------------------------------------------------------- */

/**
 * help -- Lista todos los comandos disponibles agrupados por modulo
 * o muestra la ayuda especifica si se especifica un comando.
 */
static int builtin_help(Command *cmd) {
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

        printf("  [Modulo 2 -- Hilos]\n");
        printf("    thread-demo [p] [c] [n]\n");
        printf("                      Lanza la demo Productor-Consumidor con p productores,\n");
        printf("                      c consumidores y n items por productor.\n");
        printf("                      Valores por defecto: 2 productores, 2 consumidores, 10 items.\n");
        printf("    thread-stats      Muestra estadisticas de la ultima sesion de hilos.\n");
        printf("\n");

        printf("  [Modulo 3 -- Memoria]   (disponible en Issue 3.1+)\n");
        printf("    mem-config        Configura el tamanho de la RAM simulada.\n");
        printf("    mem-view          Muestra el estado actual de la memoria.\n");
        printf("\n");
        return 0;
    }

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
        printf("\n  thread-demo [p] [c] [n]\n");
        printf("    Ejecuta la simulacion concurrente del problema clasico Productor-Consumidor.\n\n");
        printf("    Parametros opcionales (todos deben ser enteros positivos >= 2):\n");
        printf("      p  Numero de hilos productores  (defecto: 2)\n");
        printf("      c  Numero de hilos consumidores (defecto: 2)\n");
        printf("      n  Items que genera cada productor (defecto: 10)\n\n");
        printf("    Ejemplo:\n");
        printf("      thread-demo 3 2 20   # 3 productores, 2 consumidores, 20 items c/u\n");
        printf("      thread-demo          # ejecuta con valores por defecto\n\n");
        printf("    Usa un bufer circular de %d slots protegido con pthread_mutex_t y\n", BUFFER_SIZE);
        printf("    pthread_cond_t para garantizar 0 perdidas de datos.\n\n");
    } else if (strcmp(topic, "thread-stats") == 0) {
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
 * Devuelve -1 como senal a shell_loop para romper el bucle REPL.
 * Soporta un codigo numerico opcional (ej: exit 0).
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
    return -1;
}

/* -----------------------------------------------------------------------
 * builtin_thread_demo -- Issue 7.3 (SIS-62)
 *
 * Sintaxis: thread-demo [p] [c] [n]
 *   p = numero de productores  (defecto 2, minimo 2)
 *   c = numero de consumidores (defecto 2, minimo 2)
 *   n = items por productor    (defecto 10, minimo 1)
 *
 * Validaciones:
 *   - Demasiados argumentos (> 3): mensaje de uso y retorno 1.
 *   - Argumentos no numericos o negativos: error descriptivo.
 *   - p < 2 o c < 2: aviso del criterio de aceptacion del spec.
 *
 * Llama a thread_demo_run() de producer_consumer.c (Issue 7.2).
 * ----------------------------------------------------------------------- */
static int builtin_thread_demo(Command *cmd) {
    /* Valores por defecto segun el spec de la Issue 7.3 */
    int p = 2;   /* productores  */
    int c = 2;   /* consumidores */
    int n = 10;  /* items por productor */

    /* --- Validar numero de argumentos --- */
    if (cmd->argc > 4) {
        fprintf(stderr, "miniOS: thread-demo: demasiados argumentos\n");
        fprintf(stderr, "  Uso: thread-demo [p] [c] [n]\n");
        fprintf(stderr, "       p = productores (defecto 2), c = consumidores (defecto 2),\n");
        fprintf(stderr, "       n = items por productor (defecto 10)\n");
        return 1;
    }

    /* --- Parsear argumentos opcionales --- */
    if (cmd->argc >= 2) {
        char *ep = NULL;
        long val = strtol(cmd->argv[1], &ep, 10);
        if (*ep != '\0' || ep == cmd->argv[1] || val <= 0) {
            fprintf(stderr, "miniOS: thread-demo: '%s': el numero de productores debe ser un entero positivo\n",
                    cmd->argv[1]);
            return 1;
        }
        p = (int)val;
    }

    if (cmd->argc >= 3) {
        char *ep = NULL;
        long val = strtol(cmd->argv[2], &ep, 10);
        if (*ep != '\0' || ep == cmd->argv[2] || val <= 0) {
            fprintf(stderr, "miniOS: thread-demo: '%s': el numero de consumidores debe ser un entero positivo\n",
                    cmd->argv[2]);
            return 1;
        }
        c = (int)val;
    }

    if (cmd->argc >= 4) {
        char *ep = NULL;
        long val = strtol(cmd->argv[3], &ep, 10);
        if (*ep != '\0' || ep == cmd->argv[3] || val <= 0) {
            fprintf(stderr, "miniOS: thread-demo: '%s': el numero de items debe ser un entero positivo\n",
                    cmd->argv[3]);
            return 1;
        }
        n = (int)val;
    }

    /* --- Validar minimos segun criterio de aceptacion del spec --- */
    if (p < 2) {
        fprintf(stderr, "miniOS: thread-demo: se requieren minimo 2 productores (recibido: %d)\n", p);
        return 1;
    }
    if (c < 2) {
        fprintf(stderr, "miniOS: thread-demo: se requieren minimo 2 consumidores (recibido: %d)\n", c);
        return 1;
    }

    /* --- Lanzar la demo --- */
    return thread_demo_run(p, c, n);
}

/* -----------------------------------------------------------------------
 * shell_dispatch -- Enrutador central de comandos (Issue 1.4 / SIS-41)
 *
 * Flujo:
 *  1. Tokeniza 'line' usando parse_command() del parser (Issue 1.3).
 *  2. Si la linea estaba vacia, retorna 0 sin hacer nada.
 *  3. Busca argv[0] en la tabla de built-ins y lo ejecuta si lo encuentra.
 *  4. Si no es un built-in, llama a process_execute_fg (Issue 2.1).
 * ----------------------------------------------------------------------- */
int shell_dispatch(char *line) {
    Command cmd;
    int argc = parse_command(line, &cmd);

    if (argc == 0) {
        free_command(&cmd);
        return 0;
    }

    for (int i = 0; i < BUILTIN_COUNT; i++) {
        if (strcmp(cmd.argv[0], BUILTINS[i].name) == 0) {
            int result = BUILTINS[i].handler(&cmd);
            free_command(&cmd);
            return result;
        }
    }

    int result = process_execute_fg(cmd.argv);
    free_command(&cmd);
    return result;
}

/* -----------------------------------------------------------------------
 * shell_loop -- Ciclo principal REPL (Issue 1.2 / SIS-39)
 * ----------------------------------------------------------------------- */
void shell_loop(void) {
    char line[MAX_LINE];

    while (1) {
        printf("miniOS> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n[INFO] Forzando cierre, Saliendo del shell...\n");
            break;
        }

        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) == 0) continue;

        if (shell_dispatch(line) == -1) break;
    }
}
