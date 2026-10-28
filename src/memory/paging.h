#ifndef PAGING_H
#define PAGING_H

#define MAX_FRAMES 128
#define MAX_PAGES 64

typedef struct {
    int frame_number;
    int is_free;
    int pid;
} frame_t;

typedef struct {
    int page_number;
    int frame_number;
    int is_valid;
} page_entry_t;

typedef struct {
    int pid;
    int num_pages;
    page_entry_t pages[MAX_PAGES];
} page_table_t;

/**
 * @brief Inicializa la tabla de marcos globales de paginación.
 */
void paging_init(int total_ram_kb, int page_size_kb);

/**
 * @brief Asigna marcos físicos a las páginas requeridas por el proceso.
 */
int paging_alloc(int pid, int size_kb);

/**
 * @brief Libera los marcos ocupados por un proceso.
 */
int paging_free(int pid);

/**
 * @brief Traduce una dirección lógica a física.
 */
int paging_translate(int pid, int logical_address, int *out_physical_address);

/**
 * @brief Visualiza el estado de los marcos y tablas de páginas.
 */
void paging_view(void);

/**
 * @brief Libera estructuras asociadas a la paginación.
 */
void paging_cleanup(void);

#endif // PAGING_H
