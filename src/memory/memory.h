#ifndef MEMORY_H
#define MEMORY_H

#define DEFAULT_RAM_KB 128
#define DEFAULT_PAGE_KB 4

typedef enum {
    MEM_ALGO_FIRST_FIT,
    MEM_ALGO_PAGING
} mem_algorithm_t;

/* --- Issue 3.1: Estructuras para gestión de bloques de memoria --- */

typedef enum {
    BLOCK_FREE = 0,
    BLOCK_USED = 1
} BlockStatus;

typedef struct MemBlock {
    int id;
    int start_kb;
    int size_kb;
    BlockStatus status;
    int process_id;             // -1 si libre
    struct MemBlock *next;
} MemBlock;

typedef struct {
    int total_ram_kb;
    int page_size_kb;
    int total_frames;
    char algorithm[8];          // "ff" (First-Fit contiguo) o "pag" (Paginación)
    MemBlock *block_list;
} MemoryState;

extern MemoryState mem_state;

/* --- Configuración interna del subsistema --- */

typedef struct {
    int total_ram_kb;
    int page_size_kb;
    int total_frames;
    mem_algorithm_t active_algorithm;
} mem_config_t;

/**
 * @brief Inicializa mem_state con valores predeterminados (DEFAULT_RAM_KB /
 *        DEFAULT_PAGE_KB) y un único bloque libre que cubre toda la RAM.
 */
void mem_init(void);

/**
 * @brief Libera la lista enlazada de bloques y deja mem_state limpio.
 */
void mem_reset_blocks(void);

/**
 * @brief Inicializa el subsistema de memoria con parámetros predeterminados.
 */
void memory_init(int total_ram_kb, mem_algorithm_t algo);

/**
 * @brief Obtiene la configuración actual de memoria.
 */
const mem_config_t *memory_get_config(void);

/**
 * @brief Asigna memoria al proceso según el algoritmo activo.
 */
int memory_alloc(int pid, int size_kb);

/**
 * @brief Libera la memoria ocupada por un proceso.
 */
int memory_free(int pid);

/**
 * @brief Muestra el estado visual de la memoria (mem-view).
 */
void memory_view(void);

/**
 * @brief Limpia todas las estructuras y libera memoria reservada.
 */
void memory_cleanup(void);

#endif // MEMORY_H
