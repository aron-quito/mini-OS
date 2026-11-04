/*
 * shell.c -- Issue 1.2 (SIS-39) + Issue 1.4 (SIS-41) + Issue 2.2 (SIS-43)
 *           + Issue 2.3 (SIS-44) + Issue 3.1 (SIS-46) + Issue 3.2 (SIS-47)
 *           + Issue 7.1 (SIS-60) + Issue 7.3 (SIS-62) + Issue 7.4 (SIS-63)
 *
 * Issue 1.2 (Aron): Ciclo principal REPL (shell_loop).
 * Issue 1.4 (JSON): Dispatcher central con tabla de built-ins y enrutamiento
 *                   modular a los comandos del sistema (shell_dispatch).
 * Issue 2.2 (Aron): Ejecucion asincrona en background con operador '&':
 *                   extraccion del argumento de memoria (KB), bifurcacion
 *                   fg/bg en shell_dispatch y cosecha limpia antes del prompt.
 * Issue 2.3 (Aron): Tabla de control de procesos en background (bg_table).
 *                   Registra PID, PPID, comando, mem_kb y estado (RUNNING/
 *                   FINISHED/KILLED). Expone el comando ps-mini como built-in.
 * Issue 3.1 (Luis): Inicializacion del subsistema de memoria fisica simulada.
 * Issue 3.2 (Luis): Comandos mem-config (ram/page/frames/default) y mem-status.
 * Issue 7.1 (JSON): Ayuda extendida en help con comandos de hilos.
 * Issue 7.3 (JSON): Comando thread-demo [p] [c] [n] conectado a
 *                   thread_demo_run() del modulo de hilos.
 * Issue 7.4 (JSON): Comando thread-stats -- estadisticas de sesion con
 *                   verificacion de 0 perdidas de datos.
 * Issue 4.2 (JSON): Comandos mem-alloc <pid> <kb> y mem-free <pid>; encolado
 *                   automatico en mem_queue cuando la RAM es insuficiente.
 * Issue 4.3 (JSON): Comando mem-queue -- muestra tabla de procesos en espera.
 */

#include "shell.h"
#include "parser.h"
#include "../process/process.h"
#include "../memory/memory.h"
#include "../threads/producer_consumer.h" /* thread_demo_run, thread_get_stats -- Issues 7.3 / 7.4 */
#include "../memory/mem_queue.h"             /* mem_queue_print -- Issue 4.2 (SIS-51) */
#include "../memory/paging.h"                /* cmd_mem_translate -- Issue 5.2 (SIS-55) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Prototipos internos (funciones privadas de este archivo)
 * ----------------------------------------------------------------------- */
static int builtin_help(Command *cmd);
static int builtin_clear(Command *cmd);
static int builtin_exit(Command *cmd);
static int builtin_thread_demo(Command *cmd);          /* Issue 7.3 (SIS-62) */
static int builtin_thread_stats(Command *cmd);         /* Issue 7.4 (SIS-63) */
static int builtin_mem_config(Command *cmd);           /* Issue 3.2 (SIS-47) */
static int builtin_mem_status(Command *cmd);           /* Issue 3.2 (SIS-47) */
static int builtin_mem_alloc(Command *cmd);            /* Issue 4.2 (SIS-51) */
static int builtin_mem_free(Command *cmd);             /* Issue 4.2 (SIS-51) */
static int builtin_mem_queue(Command *cmd);            /* Issue 4.3 (SIS-52) */
static int builtin_mem_translate(Command *cmd);        /* Issue 5.2 (SIS-55) */
static int builtin_mem_page_table(Command *cmd);       /* Issue 5.3 (SIS-56) */
static int builtin_mem_view(Command *cmd);             /* Issue 6.1 (SIS-57) */
static int builtin_ps_mini(Command *cmd);              /* Issue 2.3 (SIS-44) */
static int extract_mem_kb(char **argv, int *argc_ptr); /* Issue 2.2 (SIS-43) */

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
    { "help",           builtin_help           },
    { "clear",          builtin_clear          },
    { "exit",           builtin_exit           },
    { "ps-mini",        builtin_ps_mini        }, /* Issue 2.3 (SIS-44) */
    { "thread-demo",    builtin_thread_demo    }, /* Issue 7.3 (SIS-62) */
    { "thread-stats",   builtin_thread_stats   }, /* Issue 7.4 (SIS-63) */
    /* Issue 3.2 (SIS-47): Configuracion de RAM y diagnostico */
    { "mem-config",     builtin_mem_config     },
    { "mem-status",     builtin_mem_status     },
    /* Issue 4.2 (SIS-51): Asignacion y liberacion de memoria con encolado reactivo */
    { "mem-alloc",      builtin_mem_alloc      },
    { "mem-free",       builtin_mem_free       },
    /* Issue 4.3 (SIS-52): Inspeccion de la cola de espera de RAM */
    { "mem-queue",      builtin_mem_queue      },
    /* Issue 5.2 / 5.3 (SIS-55 / SIS-56): Comandos de Paginacion */
    { "mem-translate",  builtin_mem_translate  },
    { "mem-page-table", builtin_mem_page_table },
    /* Issue 6.1 (SIS-57): Visualizador grafico interactivo de RAM y alias */
    { "mem-view",       builtin_mem_view       },
    { "ram-view",       builtin_mem_view       },
};

/* Numero de entradas en la tabla, calculado en tiempo de compilacion */
#define BUILTIN_COUNT ((int)(sizeof(BUILTINS) / sizeof(BUILTINS[0])))

/* -----------------------------------------------------------------------
 * shell_init -- Inicializacion de los subsistemas del sistema
 *
 * Issue 2.2 (SIS-43): Inicializa la tabla de control de background (bg_table).
 * Issue 3.1 (SIS-46): Inicializa la memoria fisica simulada con 128 KB
 * bajo el esquema de particionamiento dinamico First-Fit y muestra el
 * estado del bloque libre inicial.
 * ----------------------------------------------------------------------- */
void shell_init(void) {
    /* Issue 2.2: Inicializar subsistema de procesos en background */
    process_init();

    /* Issue 3.1: Inicializar subsistema de memoria fisica simulada */
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
    const char *topic;

    /* Si no se especifico un argumento, mostrar la ayuda general */
    if (cmd->argc <= 1) {
        printf("\n");
        printf("  miniOS -- Shell Interactivo Educativo en C11\n");
        printf("  ============================================\n\n");
        printf("  Uso: help [comando]\n\n");

        printf("  [Shell / Built-ins]\n");
        printf("    help                    Muestra este mensaje de ayuda o el detalle de un comando.\n");
        printf("    clear                   Limpia la pantalla de la terminal.\n");
        printf("    exit                    Finaliza la sesion del shell.\n");
        printf("\n");

        printf("  [Modulo 1 -- Procesos]  (Issue 2.1 / 2.2)\n");
        printf("    <cmd> [args]            Ejecuta un comando del sistema en primer plano.\n");
        printf("    <cmd> [args] [kb] &     Ejecuta en segundo plano con reserva de RAM simulada.\n");
        printf("    sleep <seg> [kb] &      Lanza una pausa asincrona simulando proceso en background.\n");
        printf("    ps-mini                 Lista los procesos activos de la sesion.\n");
        printf("\n");

        printf("  [Modulo 2 -- Hilos]\n");
        printf("    thread-demo [p] [c] [n]\n");
        printf("                            Lanza la demo Productor-Consumidor con p productores,\n");
        printf("                            c consumidores y n items por productor.\n");
        printf("                            Valores por defecto: 2 productores, 2 consumidores, 10 items.\n");
        printf("    thread-stats            Muestra estadisticas de la ultima sesion de hilos:\n");
        printf("                            items producidos, consumidos, perdidas y tiempo de ejecucion.\n");
        printf("\n");

        printf("  [Modulo 3 -- Memoria]   (disponible en Issue 3.1+)\n");
        printf("    mem-config ram <KB>      Configura la RAM total; reinicia el bloque libre.\n");
        printf("    mem-config page <KB>     Configura el tamano de pagina (potencia de 2).\n");
        printf("    mem-config frames <N>    Configura N marcos; recalcula RAM = N x page_size.\n");
        printf("    mem-config default       Preset: 1024 KB RAM, 64 KB/pagina, 16 marcos, FF.\n");
        printf("    mem-status               Muestra el resumen del estado de la memoria.\n");
        printf("    mem-alloc <pid> <kb>     Asigna memoria RAM a un proceso o lo encola.\n");
        printf("    mem-free <pid>           Libera la memoria ocupada por un proceso.\n");
        printf("    mem-translate <pid> <d>  Traduce una direccion logica a fisica (modo PAG).\n");
        printf("    mem-page-table <pid>     Visualiza la tabla de paginas del proceso (modo PAG).\n");
        printf("    mem-view / ram-view      Muestra el mapa visual de RAM y cola de espera.\n");
        printf("\n");
        return 0;
    }

    /* Ayuda detallada para un comando especifico */
    topic = cmd->argv[1];

    if (strcmp(topic, "help") == 0) {
        printf("\n  help [comando]\n");
        printf("    Muestra informacion de ayuda general o el detalle de un comando especifico.\n\n");
    } else if (strcmp(topic, "clear") == 0) {
        printf("\n  clear\n");
        printf("    Limpia el contenido de la pantalla de la terminal mediante secuencias ANSI.\n\n");
    } else if (strcmp(topic, "exit") == 0) {
        printf("\n  exit [codigo]\n");
        printf("    Finaliza la sesion de miniOS. Opcionalmente acepta un codigo numerico de salida.\n\n");
    } else if (strcmp(topic, "sleep") == 0) {
        printf("\n  sleep <segundos> [mem_kb] &\n");
        printf("    Ejecuta un proceso de espera en segundo plano simulando carga y memoria (Issue 2.2).\n");
        printf("    Parametros:\n");
        printf("      <segundos>  Tiempo que el proceso permanecera activo.\n");
        printf("      [mem_kb]    (Opcional) Kilobytes de RAM simulada a reservar (por defecto: %d KB).\n", DEFAULT_BG_MEM_KB);
        printf("      &           Obligatorio para ejecucion asincrona en background.\n");
        printf("    Ejemplos:\n");
        printf("      sleep 5 &\n");
        printf("      sleep 10 256 &\n\n");
    } else if (strcmp(topic, "ps-mini") == 0) {
        printf("\n  ps-mini\n");
        printf("    Muestra la tabla de procesos en background de la sesion actual (Issue 2.4).\n\n");
        printf("    Columnas:\n");
        printf("      #          Numero de orden en la tabla (1-indexed).\n");
        printf("      PID        ID del proceso hijo lanzado.\n");
        printf("      PPID       ID del proceso padre (el shell, siempre el mismo).\n");
        printf("      Comando    Cadena del comando ejecutado (truncada a 20 caracteres).\n");
        printf("      RAM (KB)   Kilobytes reservados en la RAM simulada.\n");
        printf("                 Muestra 0 KB si el proceso ya termino y la memoria fue liberada.\n");
        printf("      Estado     RUNNING  = proceso activo en este momento.\n");
        printf("                 DONE(N)  = proceso terminado con codigo de salida N.\n");
        printf("                 KILLED(S)= proceso terminado por la senal S.\n\n");
        printf("    Formato de salida:\n");
        printf("      Tabla Unicode con bordes dobles (UTF-8). Hasta %d entradas.\n\n", MAX_BG_PROCESSES);
        printf("    Ejemplo:\n");
        printf("      miniOS> sleep 10 256 &\n");
        printf("      miniOS> ls &\n");
        printf("      miniOS> ps-mini\n\n");
        printf("      Nota: solo registra procesos lanzados con '&'. Los procesos en\n");
        printf("      foreground no aparecen en la tabla.\n\n");
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
        printf("\n  mem-config <sub-comando> [valor]\n");
        printf("    Configura los parametros de la memoria fisica simulada.\n\n");
        printf("    Sub-comandos:\n");
        printf("      ram <KB>      Redefine la RAM total. Requisito: multiplo de page_size.\n");
        printf("      page <KB>     Tamano de pagina (potencia de 2). Latente en modo FF.\n");
        printf("      frames <N>    N marcos; RAM = N * page_size_kb.\n");
        printf("      default       Preset: 1024 KB RAM, 64 KB/pag, 16 marcos, First-Fit.\n\n");
        printf("    Restricciones:\n");
        printf("      - Rechazado si hay procesos activos en memoria.\n");
        printf("      - En modo FF, 'mem-page-table' y 'mem-translate' estan bloqueados.\n\n");
    } else if (strcmp(topic, "mem-status") == 0) {
        printf("\n  mem-status\n");
        printf("    Muestra el estado actual de la memoria: RAM total, algoritmo,\n");
        printf("    bloques libres/usados (FF) o marcos (PAG), y porcentaje de uso.\n\n");
    } else if (strcmp(topic, "mem-alloc") == 0) {
        printf("\n  mem-alloc <pid> <kb>\n");
        printf("    Asigna <kb> kilobytes de RAM al proceso con PID <pid>.\n");
        printf("    Si no hay espacio contiguo disponible, el proceso se encola\n");
        printf("    automaticamente en la cola FIFO de espera (mem_queue).\n\n");
        printf("    Parametros:\n");
        printf("      <pid>  Identificador del proceso (entero positivo).\n");
        printf("      <kb>   Kilobytes de RAM solicitados (entero positivo).\n\n");
        printf("    Ejemplos:\n");
        printf("      mem-alloc 42 128    # asigna 128 KB al PID 42\n");
        printf("      mem-alloc 99 2048   # encola si no hay 2048 KB libres\n\n");
    } else if (strcmp(topic, "mem-free") == 0) {
        printf("\n  mem-free <pid>\n");
        printf("    Libera todos los bloques de RAM asignados al proceso <pid>.\n");
        printf("    Aplica coalescencia de bloques adyacentes libres (Issue 3.4).\n\n");
        printf("    Parametros:\n");
        printf("      <pid>  PID del proceso cuya memoria se libera.\n\n");
        printf("    Ejemplo:\n");
        printf("      mem-free 42\n\n");
    } else if (strcmp(topic, "mem-queue") == 0) {
        printf("\n  mem-queue\n");
        printf("    Muestra la tabla de procesos que estan esperando RAM.\n");
        printf("    Por cada proceso muestra: posicion en la fila, PID, KB solicitados\n");
        printf("    y el tiempo que lleva esperando en formato mm:ss.\n\n");
        printf("    Muestra aviso si la cola esta vacia.\n\n");
        printf("    Ejemplo:\n");
        printf("      miniOS> mem-alloc 42 90   # asigna\n");
        printf("      miniOS> mem-alloc 99 999  # encola (no cabe)\n");
        printf("      miniOS> mem-queue         # muestra la tabla\n\n");
    } else if (strcmp(topic, "mem-translate") == 0) {
        printf("\n  mem-translate <pid> <dir_logica>\n");
        printf("    Traduce una direccion logica (KB) a su direccion fisica correspondiente\n");
        printf("    usando la tabla de paginas del proceso (exclusivo del modo Paginacion).\n\n");
        printf("    Formulacion:\n");
        printf("      num_pagina   = dir_logica / page_size_kb\n");
        printf("      offset       = dir_logica %% page_size_kb\n");
        printf("      dir_fisica   = (marco_fisico * page_size_kb) + offset\n\n");
        printf("    Ejemplo:\n");
        printf("      mem-translate 42 192\n\n");
    } else if (strcmp(topic, "mem-page-table") == 0) {
        printf("\n  mem-page-table <pid>\n");
        printf("    Visualiza la tabla de paginas asociada al proceso <pid> (exclusivo de Paginacion).\n");
        printf("    Muestra para cada pagina logica su marco fisico, estado de validez (SI/NO)\n");
        printf("    y su direccion base fisica (marco_fisico * page_size_kb).\n\n");
        printf("    Ejemplo:\n");
        printf("      mem-page-table 42\n\n");
    } else if (strcmp(topic, "mem-view") == 0 || strcmp(topic, "ram-view") == 0) {
        printf("\n  mem-view (alias: ram-view)\n");
        printf("    Renderiza en terminal el mapa grafico de la memoria RAM adaptado al\n");
        printf("    algoritmo activo (tira de bloques en First-Fit o cuadricula de marcos\n");
        printf("    en Paginacion), metricas de fragmentacion y estado de mem_queue.\n\n");
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
 * Devuelve un codigo especial (-1) que shell_dispatch reenvia a
 * shell_loop para que sepa que debe romper el bucle REPL.
 * Soporta un codigo numerico opcional (ej: exit 0). Si recibe texto
 * o argumentos invalidos, emite un error y no finaliza la sesion.
 */
static int builtin_exit(Command *cmd) {
    char *endptr;
    long  code;

    if (cmd->argc > 2) {
        fprintf(stderr, "miniOS: exit: demasiados argumentos\n");
        return 1;
    }

    if (cmd->argc == 2) {
        endptr = NULL;
        code   = strtol(cmd->argv[1], &endptr, 10);
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

/**
 * ps-mini — Muestra la tabla de control de procesos del background.
 *
 * Delega directamente en process_list_mini() (Issue 2.3 / SIS-44).
 * No acepta argumentos; si se pasan, los ignora.
 */
static int builtin_ps_mini(Command *cmd) {
    (void)cmd;
    process_list_mini();
    return 0;
}

/* -----------------------------------------------------------------------
 * builtin_thread_demo -- Issue 7.3 (SIS-62)
 *
 * Sintaxis: thread-demo [p] [c] [n]
 *   p = numero de productores  (defecto 2, minimo 2)
 *   c = numero de consumidores (defecto 2, minimo 2)
 *   n = items por productor    (defecto 10, minimo 1)
 *
 * Llama a thread_demo_run() de producer_consumer.c (Issue 7.2).
 * ----------------------------------------------------------------------- */
static int builtin_thread_demo(Command *cmd) {
    int p = 2;   /* Productores por defecto */
    int c = 2;   /* Consumidores por defecto */
    int n = 10;  /* Items por productor por defecto */

    if (cmd->argc > 4) {
        fprintf(stderr, "miniOS: thread-demo: demasiados argumentos\n");
        fprintf(stderr, "Uso: thread-demo [num_productores] [num_consumidores] [items_por_productor]\n");
        return 1;
    }

    if (cmd->argc >= 2) {
        char *ep = NULL;
        long val = strtol(cmd->argv[1], &ep, 10);
        if (*ep != '\0' || ep == cmd->argv[1] || val <= 0) {
            fprintf(stderr, "miniOS: thread-demo: '%s': el numero de productores debe ser un entero positivo\n", cmd->argv[1]);
            return 1;
        }
        p = (int)val;
    }

    if (cmd->argc >= 3) {
        char *ep = NULL;
        long val = strtol(cmd->argv[2], &ep, 10);
        if (*ep != '\0' || ep == cmd->argv[2] || val <= 0) {
            fprintf(stderr, "miniOS: thread-demo: '%s': el numero de consumidores debe ser un entero positivo\n", cmd->argv[2]);
            return 1;
        }
        c = (int)val;
    }

    if (cmd->argc >= 4) {
        char *ep = NULL;
        long val = strtol(cmd->argv[3], &ep, 10);
        if (*ep != '\0' || ep == cmd->argv[3] || val <= 0) {
            fprintf(stderr, "miniOS: thread-demo: '%s': el numero de items debe ser un entero positivo\n", cmd->argv[3]);
            return 1;
        }
        n = (int)val;
    }

    if (p < 2) {
        fprintf(stderr, "miniOS: thread-demo: se requieren minimo 2 productores (recibido: %d)\n", p);
        return 1;
    }
    if (c < 2) {
        fprintf(stderr, "miniOS: thread-demo: se requieren minimo 2 consumidores (recibido: %d)\n", c);
        return 1;
    }

    return thread_demo_run(p, c, n);
}

/* -----------------------------------------------------------------------
 * builtin_thread_stats -- Issue 7.4 (SIS-63)
 *
 * Sintaxis: thread-stats
 *
 * Muestra las estadisticas de la ultima sesion de hilos ejecutada con
 * thread-demo. Si no se ha ejecutado ninguna sesion, informa al usuario.
 * ----------------------------------------------------------------------- */
static int builtin_thread_stats(Command *cmd) {
    (void)cmd; /* thread-stats no acepta argumentos */

    const thread_stats_t *s = thread_get_stats();

    if (s == NULL) {
        printf("\n  [INFO] No hay sesion de hilos registrada.\n");
        printf("         Ejecuta 'thread-demo' primero y luego consulta las estadisticas.\n\n");
        return 0;
    }

    int loss = s->items_produced - s->items_consumed;

    printf("\n");
    printf("  +======================================================+\n");
    printf("  |         ESTADISTICAS -- Sesion de Hilos              |\n");
    printf("  +======================================================+\n");
    printf("  |  Productores        : %-30d|\n", s->producers_count);
    printf("  |  Consumidores       : %-30d|\n", s->consumers_count);
    printf("  |  Items producidos   : %-30d|\n", s->items_produced);
    printf("  |  Items consumidos   : %-30d|\n", s->items_consumed);

    if (loss == 0) {
        printf("  |  Perdida de datos   : 0  [OK - NINGUNA]             |\n");
    } else {
        printf("  |  Perdida de datos   : %-30d|\n", loss);
        printf("  |  [ADVERTENCIA] Perdida detectada -- revisar sync!    |\n");
    }

    printf("  |  Capacidad del bufer: %-22d slots  |\n", BUFFER_SIZE);
    printf("  |  Tiempo de computo  : %-24.3f seg  |\n", s->execution_time_sec);
    printf("  +======================================================+\n");
    printf("\n");

    /* Retorna 1 si hubo perdida (para saber que algo fallo en las pruebas) */
    return (loss != 0) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 * Issue 3.2 (SIS-47) -- Handlers de comandos de memoria
 *
 * Adaptan la firma (Command*) de la tabla de built-ins a la API de
 * memory.c (tipos primitivos), manteniendo la separacion entre la capa
 * de presentacion CLI y la logica del kernel de memoria.
 * ----------------------------------------------------------------------- */

static int builtin_mem_config(Command *cmd) {
    if (cmd->argc < 2) {
        fprintf(stderr,
            "Uso: mem-config <sub-comando> [valor]\n"
            "  mem-config ram <KB>      Configura la RAM total en KB.\n"
            "  mem-config page <KB>     Configura el tamano de pagina en KB (potencia de 2).\n"
            "  mem-config frames <N>    Configura N marcos; recalcula RAM = N * page_size.\n"
            "  mem-config default       Preset: 1024 KB RAM, 64 KB/pagina, 16 marcos, FF.\n");
        return 1;
    }

    const char *subcmd = cmd->argv[1];

    /* Sub-comando "default": no requiere valor numerico */
    if (strcmp(subcmd, "default") == 0) {
        return memory_config_default();
    }

    /* Sub-comandos que requieren un valor numerico en argv[2] */
    if (cmd->argc < 3) {
        fprintf(stderr,
            "[ERROR] 'mem-config %s' requiere un valor. Uso: mem-config %s <valor>\n",
            subcmd, subcmd);
        return 1;
    }

    /* Parseo defensivo del valor numerico */
    char *endptr = NULL;
    long val = strtol(cmd->argv[2], &endptr, 10);
    if (*endptr != '\0' || endptr == cmd->argv[2] || val <= 0 || val > 1048576) {
        fprintf(stderr,
            "[ERROR] Valor invalido '%s'. Debe ser un entero positivo.\n",
            cmd->argv[2]);
        return 1;
    }

    if (strcmp(subcmd, "ram")    == 0) return memory_config_ram((int)val);
    if (strcmp(subcmd, "page")   == 0) return memory_config_page((int)val);
    if (strcmp(subcmd, "frames") == 0) return memory_config_frames((int)val);

    fprintf(stderr,
        "[ERROR] Sub-comando desconocido: '%s'\n"
        "        Usa 'mem-config' sin argumentos para ver las opciones disponibles.\n",
        subcmd);
    return 1;
}

static int builtin_mem_status(Command *cmd) {
    (void)cmd;
    memory_status();
    return 0;
}

/* -----------------------------------------------------------------------
 * Issue 4.2 (SIS-51) -- Handlers de mem-alloc y mem-free
 *
 * builtin_mem_alloc: Parsea <pid> y <kb>, llama a memory_alloc().
 *   Si hay RAM disponible: First-Fit la asigna y muestra confirmacion.
 *   Si no hay espacio: memory_alloc encola automaticamente en mem_queue.
 *
 * builtin_mem_free: Parsea <pid>, llama a memory_free() (ff_free con coalescencia).
 * ----------------------------------------------------------------------- */

static int builtin_mem_alloc(Command *cmd) {
    if (cmd->argc < 3) {
        fprintf(stderr, "[ERROR] Uso: mem-alloc <pid> <kb>\n");
        return 1;
    }

    char *endptr1 = NULL, *endptr2 = NULL;
    long pid = strtol(cmd->argv[1], &endptr1, 10);
    long kb  = strtol(cmd->argv[2], &endptr2, 10);

    if (*endptr1 != '\0' || endptr1 == cmd->argv[1] || pid <= 0) {
        fprintf(stderr, "[ERROR] PID invalido '%s'. Debe ser un entero positivo.\n", cmd->argv[1]);
        return 1;
    }
    if (*endptr2 != '\0' || endptr2 == cmd->argv[2] || kb <= 0) {
        fprintf(stderr, "[ERROR] Tamano invalido '%s'. Debe ser un entero positivo (KB).\n", cmd->argv[2]);
        return 1;
    }

    int r = memory_alloc((int)pid, (int)kb);
    return (r == -1) ? 1 : 0;  /* 0=asignado, 1=encolado/error */
}

static int builtin_mem_free(Command *cmd) {
    if (cmd->argc < 2) {
        fprintf(stderr, "[ERROR] Uso: mem-free <pid>\n");
        return 1;
    }

    char *endptr = NULL;
    long pid = strtol(cmd->argv[1], &endptr, 10);

    if (*endptr != '\0' || endptr == cmd->argv[1] || pid <= 0) {
        fprintf(stderr, "[ERROR] PID invalido '%s'. Debe ser un entero positivo.\n", cmd->argv[1]);
        return 1;
    }

    return memory_free((int)pid) == 0 ? 0 : 1;
}

/* -----------------------------------------------------------------------
 * Issue 4.3 (SIS-52) -- Handler de mem-queue
 *
 * Muestra la tabla de procesos en espera de RAM llamando a mem_queue_print().
 * No recibe argumentos; los ignora si se pasan.
 * ----------------------------------------------------------------------- */

static int builtin_mem_queue(Command *cmd) {
    (void)cmd;
    mem_queue_print();
    return 0;
}

/* -----------------------------------------------------------------------
 * Issue 5.2 (SIS-55) -- Handler de mem-translate <pid> <dir_logica>
 * ----------------------------------------------------------------------- */
static int builtin_mem_translate(Command *cmd) {
    if (memory_is_first_fit()) {
        fprintf(stderr,
                "[ERROR] 'mem-translate' solo esta disponible en modo Paginacion.\n"
                "        Algoritmo activo: First-Fit (particionamiento dinamico contiguo).\n"
                "        Cambia a modo PAG con 'switch-ram pag' para usar este comando.\n");
        return 1;
    }

    if (cmd->argc < 3) {
        fprintf(stderr, "[ERROR] Uso: mem-translate <pid> <dir_logica>\n");
        return 1;
    }

    char *endptr1 = NULL, *endptr2 = NULL;
    long pid        = strtol(cmd->argv[1], &endptr1, 10);
    long dir_logica = strtol(cmd->argv[2], &endptr2, 10);

    if (*endptr1 != '\0' || endptr1 == cmd->argv[1] || pid <= 0) {
        fprintf(stderr, "[ERROR] PID invalido '%s'. Debe ser un entero positivo.\n", cmd->argv[1]);
        return 1;
    }
    if (*endptr2 != '\0' || endptr2 == cmd->argv[2]) {
        fprintf(stderr, "[ERROR] Direccion logica invalida '%s'. Debe ser un valor numerico en KB.\n",
                cmd->argv[2]);
        return 1;
    }

    return (cmd_mem_translate((int)pid, (int)dir_logica) == 0) ? 0 : 1;
}

/* -----------------------------------------------------------------------
 * Issue 5.3 (SIS-56) -- Handler de mem-page-table <pid>
 * ----------------------------------------------------------------------- */
static int builtin_mem_page_table(Command *cmd) {
    return cmd_mem_page_table(cmd->argc, cmd->argv);
}

/* -----------------------------------------------------------------------
 * Issue 6.1 (SIS-57) -- Handler de mem-view y ram-view
 * ----------------------------------------------------------------------- */
static int builtin_mem_view(Command *cmd) {
    return cmd_mem_view(cmd->argc, cmd->argv);
}

/* -----------------------------------------------------------------------
 * extract_mem_kb -- Issue 2.2 (SIS-43)
 *
 * Extrae el argumento de memoria del arreglo de argumentos.
 *
 * Cuando el usuario ejecuta un comando en background con tamanho explicito:
 *   sleep 10 256 &   ->   argv = ["sleep", "10", "256", NULL], is_background=1
 *
 * El ultimo argumento numerico ("256") es el tamanho de RAM en KB que pide el
 * proceso. Esta funcion lo detecta, lo extrae del arreglo argv y ajusta argc.
 * Si el ultimo argumento no es un numero puro, usa DEFAULT_BG_MEM_KB (128 KB).
 * ----------------------------------------------------------------------- */
static int extract_mem_kb(char **argv, int *argc_ptr) {
    char *last;
    char *endptr;
    long  val;
    int   argc = *argc_ptr;

    /*
     * Se necesita al menos: argv[0]=comando, argv[1]=argumento, argv[2]=KB.
     * Con argc < 3 no hay forma de distinguir el unico argumento numerico
     * (que pertenece al comando) del argumento de KB de RAM.
     */
    if (argc < 3) {
        return DEFAULT_BG_MEM_KB;
    }

    /* Verificar si el ultimo argumento es un entero puro positivo */
    last   = argv[argc - 1];
    endptr = NULL;
    val    = strtol(last, &endptr, 10);

    if (*endptr == '\0' && endptr != last && val > 0) {
        /* Es un numero: extraerlo del argv y ajustar argc */
        argv[argc - 1] = NULL;
        (*argc_ptr)--;
        return (int)val;
    }

    return DEFAULT_BG_MEM_KB;
}

/* -----------------------------------------------------------------------
 * shell_dispatch -- Enrutador central de comandos (Issues 1.4 + 2.2 + 3.2)
 *
 * Flujo:
 *  1. Tokeniza 'line' usando parse_command() (Issue 1.3).
 *  2. Si la linea estaba vacia, retorna 0.
 *  3. Busca argv[0] en la tabla de built-ins y lo ejecuta si lo encuentra.
 *  4. Bloquea comandos de paginacion si el modo activo es First-Fit (Issue 3.2).
 *  5. Si no es un built-in y cmd.is_background == 0:
 *       -> process_execute_fg() -- sincrono (Issue 2.1).
 *  6. Si no es un built-in y cmd.is_background == 1:
 *       -> Extrae el argumento de KB con extract_mem_kb().
 *       -> process_execute_bg() -- asincrono con reserva de RAM (Issue 2.2).
 *
 * @param line  Linea de texto cruda leida por shell_loop.
 * @return 0 en exito, -1 si el shell debe terminar (comando exit).
 * ----------------------------------------------------------------------- */
int shell_dispatch(char *line) {
    Command cmd;
    int     argc;
    int     i;
    int     result;
    int     mem_kb;

    argc = parse_command(line, &cmd);

    /* Linea vacia o solo espacios: no hacer nada */
    if (argc == 0) {
        free_command(&cmd);
        return 0;
    }

    /* Buscar argv[0] en la tabla de built-ins */
    for (i = 0; i < BUILTIN_COUNT; i++) {
        if (strcmp(cmd.argv[0], BUILTINS[i].name) == 0) {
            result = BUILTINS[i].handler(&cmd);
            free_command(&cmd);
            return result;
        }
    }

    /*
     * Issue 3.2 (SIS-47) -- Bloqueo de comandos de paginacion en modo First-Fit.
     * mem-page-table y mem-translate son exclusivos del modo PAG.
     * Si el algoritmo activo es "ff", se rechazan con un mensaje explicativo.
     */
    if (memory_is_first_fit()) {
        if (strcmp(cmd.argv[0], "mem-page-table") == 0 ||
            strcmp(cmd.argv[0], "mem-translate")  == 0) {
            fprintf(stderr,
                "[ERROR] '%s' solo esta disponible en modo Paginacion.\n"
                "        Algoritmo activo: First-Fit (particionamiento dinamico contiguo).\n"
                "        Cambia a modo PAG con 'switch-ram pag' para usar este comando.\n",
                cmd.argv[0]);
            free_command(&cmd);
            return 1;
        }
    }

    if (cmd.is_background) {
        /*
         * Issue 2.2 -- Ejecucion en background con operador '&':
         * Extraer el argumento de KB antes de llamar a execvp para que el
         * programa externo no lo reciba como argumento desconocido.
         * Ejemplo: "sleep 10 256 &" -> execvp("sleep", ["sleep", "10", NULL])
         *                              mem_kb = 256
         */
        mem_kb = extract_mem_kb(cmd.argv, &cmd.argc);
        result = process_execute_bg(cmd.argv, mem_kb);
    } else {
        /* Issue 2.1 -- Ejecucion en foreground sincrona */
        result = process_execute_fg(cmd.argv);
    }

    free_command(&cmd);
    return result;
}

/* -----------------------------------------------------------------------
 * shell_loop -- Ciclo principal REPL (Issue 1.2 + Issue 2.2)
 *
 * Lee lineas del usuario, las limpia y las envia a shell_dispatch.
 * Antes de mostrar cada prompt, llama a process_reap_bg() para cosechar
 * procesos en background que hayan terminado. Esto garantiza que las
 * notificaciones de fin aparezcan en una linea limpia y nunca se solapen
 * con el texto que el usuario esta escribiendo.
 *
 * Si dispatch devuelve -1 (senal de exit), rompe el bucle.
 * ----------------------------------------------------------------------- */
void shell_loop(void) {
    char line[MAX_LINE];

    while (1) {
        /*
         * Cosechar procesos en background terminados ANTES del prompt.
         * Garantiza que la notificacion salga en una linea limpia y no
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

        /* Elimina '\n' o '\r\n' al final de la linea leida */
        line[strcspn(line, "\r\n")] = '\0';

        /* Ignora lineas completamente vacias (solo Enter) */
        if (strlen(line) == 0) continue;

        /* Despacha el comando; si retorna -1 es senal de exit */
        if (shell_dispatch(line) == -1) break;
    }
}
