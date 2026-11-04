#ifndef PAGING_H
#define PAGING_H

/* =========================================================================
 * Issue 5.1 (SIS-54): Estructuras de paginación y asignación de marcos
 * ========================================================================= */

#define MAX_PAGES 256
#define MAX_PROC  32

/**
 * @brief Entrada individual en la tabla de páginas de un proceso.
 */
typedef struct {
    int frame_number;   /* Número de marco físico asignado (-1 si inválido) */
    int valid;          /* 1 si la página está mapeada en RAM, 0 si no */
} PageEntry;

/**
 * @brief Tabla de páginas por proceso.
 */
typedef struct {
    int process_id;             /* PID del proceso dueño (-1 si el slot está libre) */
    int num_pages;              /* Cantidad de páginas lógicas asignadas al proceso */
    PageEntry pages[MAX_PAGES]; /* Mapeo de página lógica -> marco físico */
} PageTable;

/**
 * @brief Tabla global de marcos físicos de la RAM simulada.
 */
typedef struct {
    int total_frames;           /* Total de marcos físicos (total_ram_kb / page_size_kb) */
    int free_frames;            /* Cantidad de marcos actualmente disponibles */
    int frame_owner[MAX_PAGES]; /* -1 si libre, PID del proceso si está ocupado */
} FrameTable;

/* Variables globales del subsistema de paginación (Issue 5.1) */
extern FrameTable frame_table;
extern PageTable  page_tables[MAX_PROC];

/**
 * @brief Inicializa la tabla de marcos globales y las tablas de páginas de procesos.
 *
 * @param total_ram_kb Capacidad total de RAM en KB.
 * @param page_size_kb Tamaño de cada página/marco en KB.
 */
void paging_init(int total_ram_kb, int page_size_kb);

/**
 * @brief Inicializa la topología de paginación usando la configuración actual de mem_state (Issue 6.3).
 */
void pag_init(void);

/**
 * @brief Limpia e invalida todas las tablas de páginas y resetea FrameTable a estado libre (Issue 6.3).
 */
void pag_cleanup(void);

/**
 * @brief Asigna marcos libres (sin requerir contigüidad física) para un proceso — Issue 5.1.
 *
 * Calcula el número de páginas con división techo:
 *   pages_needed = (size_kb + page_size_kb - 1) / page_size_kb
 * Si no hay suficientes marcos libres, encola la solicitud en mem_queue y retorna -1.
 *
 * @param pid     PID del proceso solicitante.
 * @param size_kb Tamaño solicitado en KB (> 0).
 * @return 0 en éxito, -1 si los marcos son insuficientes o ocurre un error.
 */
int pag_alloc(int pid, int size_kb);

/**
 * @brief Alias compatible de pag_alloc.
 */
int paging_alloc(int pid, int size_kb);

/**
 * @brief Libera los marcos ocupados por un proceso, invalida su PageTable
 *        y despierta procesos en espera (mem_queue_process_waiting) — Issue 5.3.
 *
 * @param pid PID del proceso cuyos marcos se liberarán.
 */
void pag_free(int pid);

/**
 * @brief Libera los marcos ocupados por un proceso retornando estado (Issue 5.3).
 *
 * @param pid PID del proceso.
 * @return 0 si se liberó exitosamente, -1 si el PID no tenía páginas asignadas.
 */
int paging_free(int pid);

/**
 * @brief Visualiza la tabla de páginas de un proceso (comando mem-page-table <pid>) — Issue 5.3.
 *
 * @param argc Cantidad de argumentos del comando.
 * @param argv Arreglo de argumentos (argv[1] = "<pid>").
 * @return 0 en éxito, 1 en error de validación o proceso inexistente.
 */
int cmd_mem_page_table(int argc, char **argv);

/**
 * @brief Intenta asignar memoria a los procesos en el frente de mem_queue (Issue 4.4 / 5.3).
 */
void mem_queue_process_waiting(void);

/**
 * @brief Traduce una dirección lógica (en KB) a dirección física e imprime el desglose — Issue 5.2.
 *
 * Formulación:
 *   num_pagina   = dir_logica / page_size_kb
 *   offset       = dir_logica % page_size_kb
 *   marco_fisico = PageTable[pid].pages[num_pagina].frame_number
 *   dir_fisica   = (marco_fisico * page_size_kb) + offset
 *
 * @param pid          PID del proceso dueño del espacio lógico.
 * @param dir_logica   Dirección lógica en KB (>= 0).
 * @return 0 en éxito, -1 en error (modo FF activo, PID inexistente o fallo de segmentación).
 */
int cmd_mem_translate(int pid, int dir_logica);

/**
 * @brief Traduce una dirección lógica a física (Issue 5.2).
 */
int paging_translate(int pid, int logical_address, int *out_physical_address);

/**
 * @brief Visualiza el estado de los marcos y tablas de páginas (Issue 5.3 / 6.1).
 */
void paging_view(void);

/**
 * @brief Reinicia las estructuras asociadas a la paginación.
 */
void paging_cleanup(void);

/**
 * @brief Verifica si hay suficientes marcos libres para asignar size_kb.
 * @return 1 si cabe, 0 si no cabe.
 */
int paging_can_alloc(int size_kb);

#endif // PAGING_H
