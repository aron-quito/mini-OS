#ifndef MEMORY_H
#define MEMORY_H

#define DEFAULT_RAM_KB 128
#define DEFAULT_PAGE_KB 4

typedef enum {
    MEM_ALGO_FIRST_FIT,
    MEM_ALGO_PAGING
} mem_algorithm_t;

typedef struct {
    int total_ram_kb;
    int page_size_kb;
    int total_frames;
    mem_algorithm_t active_algorithm;
} mem_config_t;

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
