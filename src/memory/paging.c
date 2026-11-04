/**
 * @file paging.c
 * @brief Implementación del esquema de Paginación y asignación de marcos (Issue 5.1 / SIS-54).
 *
 * Gestiona la tabla global de marcos físicos (frame_table) y las tablas de páginas
 * por proceso (page_tables), asignando marcos dispersos sin requerir contigüidad física.
 */

#include <stdio.h>
#include "paging.h"
#include "memory.h"
#include "mem_queue.h"

/* Estructuras globales de paginación (Issue 5.1) */
FrameTable frame_table;
PageTable  page_tables[MAX_PROC];

/* Bandera interna para saber si las estructuras ya fueron inicializadas */
static int s_paging_initialized = 0;

/**
 * @brief Inicializa o reinicia la tabla global de marcos y las tablas de páginas.
 */
void paging_init(int total_ram_kb, int page_size_kb) {
    int frames = 0;
    if (page_size_kb > 0 && total_ram_kb > 0) {
        frames = total_ram_kb / page_size_kb;
    }
    if (frames > MAX_PAGES) {
        frames = MAX_PAGES;
    }

    frame_table.total_frames = frames;
    frame_table.free_frames  = frames;

    for (int i = 0; i < MAX_PAGES; i++) {
        frame_table.frame_owner[i] = -1;
    }

    for (int p = 0; p < MAX_PROC; p++) {
        page_tables[p].process_id = -1;
        page_tables[p].num_pages  = 0;
        for (int j = 0; j < MAX_PAGES; j++) {
            page_tables[p].pages[j].frame_number = -1;
            page_tables[p].pages[j].valid        = 0;
        }
    }

    s_paging_initialized = 1;
}

/**
 * @brief Busca el índice en page_tables para un PID existente o el primer slot libre.
 *
 * @param pid PID a buscar.
 * @return Índice [0..MAX_PROC-1], o -1 si no hay espacio disponible.
 */
static int find_or_claim_page_table_slot(int pid) {
    int free_slot = -1;
    for (int i = 0; i < MAX_PROC; i++) {
        if (page_tables[i].process_id == pid) {
            return i;
        }
        if (free_slot == -1 && page_tables[i].process_id == -1) {
            free_slot = i;
        }
    }
    return free_slot;
}

/**
 * @brief Asigna marcos libres sin requerir contigüidad física — Issue 5.1.
 *
 * 1. Calcula el número de páginas con división techo:
 *      pages_needed = (size_kb + page_size - 1) / page_size
 * 2. Si los marcos libres son insuficientes, encola el proceso en mem_queue y retorna -1.
 * 3. Recorre frame_table buscando cualquier marco con frame_owner[f] == -1,
 *    lo asigna al PID y registra el mapeo en page_tables[slot].pages[p].
 */
int pag_alloc(int pid, int size_kb) {
    if (size_kb <= 0) {
        fprintf(stderr, "[PAG] Error: El tamaño solicitado debe ser mayor a 0 KB.\n");
        return -1;
    }

    int page_size = (mem_state.page_size_kb > 0) ? mem_state.page_size_kb : DEFAULT_PAGE_KB;

    /* Inicialización perezosa si aún no se había sincronizado frame_table */
    if (!s_paging_initialized || frame_table.total_frames <= 0) {
        int ram_kb = (mem_state.total_ram_kb > 0) ? mem_state.total_ram_kb : DEFAULT_RAM_KB;
        paging_init(ram_kb, page_size);
    }

    /* 1. Cálculo de páginas requeridas mediante división techo */
    int pages_needed = (size_kb + page_size - 1) / page_size;

    if (pages_needed > MAX_PAGES) {
        fprintf(stderr,
                "[PAG] Error: El proceso PID %d requiere %d páginas, excediendo MAX_PAGES (%d).\n",
                pid, pages_needed, MAX_PAGES);
        return -1;
    }

    /* 2. Si no hay suficientes marcos libres, recurrir a la cola de espera (mem_queue) */
    if (frame_table.free_frames < pages_needed) {
        char cmd_label[32];
        snprintf(cmd_label, sizeof(cmd_label), "PID-%d (%dKB)", pid, size_kb);
        int pos = mem_queue_enqueue(pid, size_kb, cmd_label);

        printf("[ALERTA] Marcos libres insuficientes para PID %d "
               "(requiere %d páginas [%d KB], disponibles: %d marcos).\n",
               pid, pages_needed, size_kb, frame_table.free_frames);
        if (pos > 0) {
            printf("[QUEUE] Proceso PID %d encolado en cola de espera (Posición: %d).\n",
                   pid, pos);
        }
        return -1;
    }

    /* 3. Localizar un slot en page_tables para registrar la tabla del proceso */
    int slot = find_or_claim_page_table_slot(pid);
    if (slot < 0) {
        fprintf(stderr,
                "[PAG] Error: Límite de tablas de páginas alcanzado (MAX_PROC = %d).\n",
                MAX_PROC);
        return -1;
    }

    /* 4. Asignar marcos físicos libres (pueden estar dispersos / no contiguos) */
    int assigned = 0;
    for (int f = 0; f < frame_table.total_frames && assigned < pages_needed; f++) {
        if (frame_table.frame_owner[f] == -1) {
            frame_table.frame_owner[f] = pid;
            frame_table.free_frames--;

            page_tables[slot].pages[assigned].frame_number = f;
            page_tables[slot].pages[assigned].valid        = 1;
            assigned++;
        }
    }

    /* Invalidar entradas restantes de la tabla del proceso */
    for (int p = assigned; p < MAX_PAGES; p++) {
        page_tables[slot].pages[p].frame_number = -1;
        page_tables[slot].pages[p].valid        = 0;
    }

    page_tables[slot].process_id = pid;
    page_tables[slot].num_pages  = pages_needed;

    printf("[PAG] PID %d: %d KB (%d pág.) asignado en marcos [",
           pid, size_kb, pages_needed);
    for (int i = 0; i < pages_needed; i++) {
        printf("%d%s", page_tables[slot].pages[i].frame_number,
               (i + 1 < pages_needed) ? ", " : "");
    }
    printf("] | Marcos libres: %d/%d\n",
           frame_table.free_frames, frame_table.total_frames);

    return 0;
}

int paging_alloc(int pid, int size_kb) {
    return pag_alloc(pid, size_kb);
}

int paging_free(int pid) {
    (void)pid;
    // Stub base — Se implementará en Issue 5.3 (SIS-56)
    return 0;
}

int paging_translate(int pid, int logical_address, int *out_physical_address) {
    (void)pid;
    (void)logical_address;
    if (out_physical_address) {
        *out_physical_address = 0;
    }
    // Stub base — Se implementará en Issue 5.2 (SIS-55)
    return 0;
}

void paging_view(void) {
    // Stub base — Se implementará en Issue 5.3 / 6.1
}

void paging_cleanup(void) {
    paging_init(mem_state.total_ram_kb, mem_state.page_size_kb);
}
