#ifndef MEMORY_H
#define MEMORY_H

#define DEFAULT_RAM_KB 128
#define DEFAULT_PAGE_KB 4

/* Algoritmos de asignación soportados por MiniOS */
typedef enum {
    MEM_ALGO_FIRST_FIT, /* Particionamiento dinámico contiguo */
    MEM_ALGO_PAGING     /* Paginación con marcos de tamaño fijo */
} mem_algorithm_t;

/* =========================================================================
 * Issue 3.1 (SIS-46): Estructuras de datos para la lista de bloques
 * ========================================================================= */

/* Estado de asignación de un bloque de memoria */
typedef enum {
    BLOCK_FREE = 0,     /* Bloque disponible para asignar */
    BLOCK_USED = 1      /* Bloque ocupado por un proceso */
} BlockStatus;

/**
 * @brief Nodo de la lista enlazada de memoria física simulada.
 *
 * Invariante: La lista se mantiene siempre ordenada ascendentemente por start_kb.
 */
typedef struct MemBlock {
    int id;                     /* Identificador único del bloque */
    int start_kb;               /* Dirección base en KB dentro de la RAM */
    int size_kb;                /* Tamaño del bloque en KB */
    BlockStatus status;         /* BLOCK_FREE o BLOCK_USED */
    int process_id;             /* PID del dueño (-1 si está libre) */
    struct MemBlock *next;      /* Puntero al siguiente bloque contiguo */
} MemBlock;

/**
 * @brief Estado global de la memoria física simulada.
 */
typedef struct {
    int total_ram_kb;           /* Capacidad total configurada en KB */
    int page_size_kb;           /* Tamaño de página/marco en KB */
    int total_frames;           /* Cantidad de marcos (RAM / page_size) */
    char algorithm[8];          /* "ff" (First-Fit) o "pag" (Paginación) */
    MemBlock *block_list;       /* Cabeza de la lista enlazada de bloques */
} MemoryState;

/* Variable global compartida del subsistema de memoria (Issue 3.1) */
extern MemoryState mem_state;

/* Configuración persistente del subsistema */
typedef struct {
    int total_ram_kb;
    int page_size_kb;
    int total_frames;
    mem_algorithm_t active_algorithm;
} mem_config_t;

/* --- Funciones de ciclo de vida de bloques (Issue 3.1) --- */

/**
 * @brief Inicializa mem_state y crea el bloque libre gigante por defecto.
 */
void mem_init(void);

/**
 * @brief Libera la lista actual y regenera el bloque gigante libre (start_kb=0).
 */
void mem_reset_blocks(void);

/* --- Interfaz pública del subsistema de memoria --- */

/**
 * @brief Inicializa el subsistema con RAM y algoritmo específicos.
 */
void memory_init(int total_ram_kb, mem_algorithm_t algo);

/**
 * @brief Retorna la configuración activa de memoria.
 */
const mem_config_t *memory_get_config(void);

/**
 * @brief Asigna memoria a un proceso (stub para Issue 3.3 / ff_alloc).
 */
int memory_alloc(int pid, int size_kb);

/**
 * @brief Libera la memoria de un proceso (stub para Issue 3.4 / ff_free).
 */
int memory_free(int pid);

/**
 * @brief Muestra el estado visual de la memoria (stub para Issue 6.1).
 */
void memory_view(void);

/**
 * @brief Libera toda la memoria dinámica de la lista enlazada al apagar el SO.
 */
void memory_cleanup(void);

/* =========================================================================
 * Issue 3.2 (SIS-47): Comandos de configuración de RAM y presets
 * ========================================================================= */

/**
 * @brief mem-config ram <KB>
 *
 * Redefine la RAM total. Validaciones:
 *   - KB debe ser múltiplo de page_size_kb.
 *   - Sin procesos activos en memoria.
 * Regenera el bloque gigante libre y recalcula total_frames.
 *
 * @return 0 en éxito, 1 en error de validación.
 */
int memory_config_ram(int ram_kb);

/**
 * @brief mem-config page <KB>
 *
 * Establece el tamaño de página (parámetro latente para Paginación).
 * Validaciones:
 *   - Debe ser potencia de 2.
 *   - ram_kb actual debe ser múltiplo del nuevo page_size_kb.
 *   - Sin procesos activos en memoria.
 * Recalcula total_frames automáticamente.
 *
 * @return 0 en éxito, 1 en error de validación.
 */
int memory_config_page(int page_kb);

/**
 * @brief mem-config frames <N>
 *
 * Establece N marcos; recalcula RAM = N * page_size_kb.
 * Regenera el bloque gigante libre con la nueva RAM.
 *
 * @return 0 en éxito, 1 en error de validación.
 */
int memory_config_frames(int frames);

/**
 * @brief mem-config default
 *
 * Preset seguro: 1024 KB RAM, 64 KB/página, 16 marcos, First-Fit activo.
 * Rechazado si hay procesos activos en memoria.
 *
 * @return 0 en éxito, 1 si hay procesos activos.
 */
int memory_config_default(void);

/**
 * @brief mem-status
 *
 * Imprime el estado actual de la memoria adaptado al algoritmo activo:
 *   - First-Fit: bloques libres/usados, KB libre y usado.
 *   - Paginación: marcos libres/usados (futuro Issue 5.x).
 */
void memory_status(void);

/**
 * @brief Verifica si el algoritmo activo es First-Fit.
 *
 * Usado en shell_dispatch para bloquear comandos de paginación
 * cuando el modo activo es "ff".
 *
 * @return 1 si activo == First-Fit, 0 en caso contrario.
 */
int memory_is_first_fit(void);

#endif // MEMORY_H
