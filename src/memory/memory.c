/**
 * @file memory.c
 * @brief Implementación del subsistema de memoria física simulada.
 *
 * Issue 3.1 (SIS-46): Modela la RAM como una lista enlazada ordenada por start_kb,
 * arrancando con un único bloque gigante libre que cubre toda la memoria.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "first_fit.h"

/* Estado global de la memoria física simulada (Issue 3.1) */
MemoryState mem_state;

/* Configuración interna por defecto (128 KB RAM, páginas de 4 KB, First-Fit) */
static mem_config_t s_mem_config = {DEFAULT_RAM_KB, DEFAULT_PAGE_KB, 32, MEM_ALGO_FIRST_FIT};

/**
 * @brief Regenera la lista enlazada con 1 bloque gigante libre (Issue 3.1).
 *
 * 1. Libera nodos existentes para evitar fugas de memoria (memory leaks).
 * 2. Asigna un bloque que abarca desde 0 hasta total_ram_kb con status BLOCK_FREE.
 */
void mem_reset_blocks(void) {
    // 1. Liberar nodos previos de la lista
    MemBlock *current = mem_state.block_list;
    while (current != NULL) {
        MemBlock *next = current->next;
        free(current);
        current = next;
    }
    mem_state.block_list = NULL;

    // 2. Crear el bloque inicial gigante (toda la RAM libre, id=0, start=0)
    MemBlock *initial_block = (MemBlock *)malloc(sizeof(MemBlock));
    if (initial_block == NULL) {
        perror("Error de asignacion de memoria en mem_reset_blocks");
        exit(EXIT_FAILURE);
    }

    block_counter = 0;
    initial_block->id = 0;
    initial_block->start_kb = 0;
    initial_block->size_kb = mem_state.total_ram_kb;
    initial_block->status = BLOCK_FREE;
    initial_block->process_id = -1;
    initial_block->next = NULL;

    // 3. Asignar como cabeza de la lista
    mem_state.block_list = initial_block;
}

/**
 * @brief Inicializa mem_state aplicando valores predeterminados (Issue 3.1).
 */
void mem_init(void) {
    // Algoritmo por defecto: First-Fit contiguo ("ff")
    strcpy(mem_state.algorithm, "ff");
    mem_state.page_size_kb = s_mem_config.page_size_kb;
    mem_state.total_frames = s_mem_config.total_frames;

    if (mem_state.total_ram_kb <= 0) {
        mem_state.total_ram_kb = s_mem_config.total_ram_kb;
    }

    // Inicializa la lista enlazada con el bloque gigante libre
    mem_reset_blocks();
}

/**
 * @brief Punto de entrada público para configurar e inicializar la memoria.
 *
 * Sincroniza s_mem_config y mem_state con el tamaño y algoritmo solicitado.
 */
void memory_init(int total_ram_kb, mem_algorithm_t algo) {
    s_mem_config.total_ram_kb = total_ram_kb;
    s_mem_config.active_algorithm = algo;

    mem_state.total_ram_kb = total_ram_kb;
    if (algo == MEM_ALGO_FIRST_FIT) {
        strcpy(mem_state.algorithm, "ff");
    } else {
        strcpy(mem_state.algorithm, "pag");
    }

    mem_init();
}

const mem_config_t *memory_get_config(void) {
    return &s_mem_config;
}

int memory_alloc(int pid, int size_kb) {
    if (memory_is_first_fit()) {
        return ff_alloc(pid, size_kb);
    }
    // Modo paginación en Milestone 5
    return 0;
}

int memory_free(int pid) {
    if (memory_is_first_fit()) {
        return first_fit_free(pid);
    }
    return 0;
}

void memory_view(void) {
    if (memory_is_first_fit()) {
        first_fit_view();
    } else {
        printf("\n[MEM-VIEW] Modo Paginacion: %d marcos fijos de %d KB\n\n",
               mem_state.total_frames, mem_state.page_size_kb);
    }
}

/**
 * @brief Limpia todas las estructuras dinámicas al apagar el simulador.
 */
void memory_cleanup(void) {
    MemBlock *current = mem_state.block_list;
    while (current != NULL) {
        MemBlock *next = current->next;
        free(current);
        current = next;
    }
    mem_state.block_list = NULL;
}

/* =========================================================================
 * Issue 3.2 (SIS-47): Lógica de configuración de RAM y presets
 * ========================================================================= */

/* --- Helpers privados --------------------------------------------------- */

/**
 * Verifica si n es potencia de 2 y mayor que 0.
 * Truco de bitmask: n & (n-1) == 0 solo para potencias de 2.
 */
static int is_power_of_two(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

/**
 * Recorre la lista de bloques buscando al menos uno BLOCK_USED.
 * Si los hay, la reconfiguración debe ser rechazada (Issue 3.2).
 *
 * @return 1 si hay procesos activos, 0 si toda la RAM está libre.
 */
static int has_active_processes(void) {
    const MemBlock *b = mem_state.block_list;
    while (b != NULL) {
        if (b->status == BLOCK_USED) return 1;
        b = b->next;
    }
    return 0;
}

/* --- Implementación de comandos de configuración (Issue 3.2) ------------ */

int memory_is_first_fit(void) {
    return strcmp(mem_state.algorithm, "ff") == 0;
}

/**
 * mem-config ram <KB>
 *
 * Redefine la RAM total. Validaciones:
 *   1. KB > 0.
 *   2. KB es múltiplo de page_size_kb.
 *   3. No hay procesos activos en memoria.
 * Si pasa, actualiza mem_state, recalcula total_frames y regenera
 * el bloque gigante libre con mem_reset_blocks().
 */
int memory_config_ram(int ram_kb) {
    if (ram_kb <= 0) {
        fprintf(stderr, "[ERROR] 'mem-config ram' requiere un entero positivo.\n"
                        "        Uso: mem-config ram <KB>\n");
        return 1;
    }

    if (ram_kb % mem_state.page_size_kb != 0) {
        fprintf(stderr,
            "[ERROR] %d KB no es multiplo del tamano de pagina actual (%d KB).\n"
            "        Ajusta primero la pagina con 'mem-config page <KB>'.\n",
            ram_kb, mem_state.page_size_kb);
        return 1;
    }

    if (has_active_processes()) {
        fprintf(stderr,
            "[ERROR] No se puede reconfigurar la RAM: hay procesos activos en memoria.\n"
            "        Libera la memoria con 'mem-free <pid>' antes de reconfigurar.\n");
        return 1;
    }

    mem_state.total_ram_kb = ram_kb;
    mem_state.total_frames  = ram_kb / mem_state.page_size_kb; /* consistencia automática */
    mem_reset_blocks();

    printf("[OK] RAM total configurada: %d KB\n", ram_kb);
    printf("[INFO] Particionamiento dinamico: 1 bloque libre continuo de %d KB inicializado.\n", ram_kb);
    printf("[INFO] Marcos recalculados: %d (pagina = %d KB)\n",
           mem_state.total_frames, mem_state.page_size_kb);
    return 0;
}

/**
 * mem-config page <KB>
 *
 * Establece el tamaño de página (parámetro latente para Paginación).
 * Validaciones:
 *   1. page_kb > 0.
 *   2. Debe ser potencia de 2.
 *   3. ram_kb actual debe ser múltiplo del nuevo page_size_kb.
 *   4. No hay procesos activos en memoria.
 * Recalcula total_frames automáticamente. En modo FF muestra aviso de
 * que el parámetro es latente y solo activará en modo PAG.
 */
int memory_config_page(int page_kb) {
    if (page_kb <= 0) {
        fprintf(stderr, "[ERROR] 'mem-config page' requiere un entero positivo.\n"
                        "        Uso: mem-config page <KB>\n");
        return 1;
    }

    if (!is_power_of_two(page_kb)) {
        fprintf(stderr,
            "[ERROR] El tamano de pagina debe ser potencia de 2.\n"
            "        Valores validos: 4, 8, 16, 32, 64, 128, 256...\n");
        return 1;
    }

    if (mem_state.total_ram_kb % page_kb != 0) {
        fprintf(stderr,
            "[ERROR] La RAM actual (%d KB) no es multiplo del nuevo tamano de pagina (%d KB).\n"
            "        Reconfigura primero la RAM con 'mem-config ram <KB>'.\n",
            mem_state.total_ram_kb, page_kb);
        return 1;
    }

    if (has_active_processes()) {
        fprintf(stderr,
            "[ERROR] No se puede reconfigurar la pagina: hay procesos activos en memoria.\n"
            "        Libera la memoria con 'mem-free <pid>' antes de reconfigurar.\n");
        return 1;
    }

    mem_state.page_size_kb = page_kb;
    mem_state.total_frames  = mem_state.total_ram_kb / page_kb; /* consistencia automática */

    printf("[OK] Tamano de pagina establecido a: %d KB\n", page_kb);
    printf("[INFO] Marcos recalculados: %d (RAM = %d KB / pagina = %d KB)\n",
           mem_state.total_frames, mem_state.total_ram_kb, page_kb);

    if (memory_is_first_fit()) {
        printf("[INFO] Modo actual: First-Fit (particionamiento dinamico continuo sin marcos).\n");
        printf("       Este parametro solo se activara al cambiar a modo Paginacion.\n");
    }
    return 0;
}

/**
 * mem-config frames <N>
 *
 * Establece N marcos y recalcula RAM = N * page_size_kb.
 * Regenera el bloque gigante libre con la nueva RAM.
 * Validaciones:
 *   1. N > 0.
 *   2. No hay procesos activos en memoria.
 */
int memory_config_frames(int frames) {
    if (frames <= 0) {
        fprintf(stderr, "[ERROR] 'mem-config frames' requiere un entero positivo.\n"
                        "        Uso: mem-config frames <N>\n");
        return 1;
    }

    if (has_active_processes()) {
        fprintf(stderr,
            "[ERROR] No se puede reconfigurar los marcos: hay procesos activos en memoria.\n"
            "        Libera la memoria con 'mem-free <pid>' antes de reconfigurar.\n");
        return 1;
    }

    int new_ram_kb = frames * mem_state.page_size_kb;
    mem_state.total_frames  = frames;
    mem_state.total_ram_kb  = new_ram_kb;
    mem_reset_blocks();

    printf("[OK] Marcos configurados: %d | RAM recalculada: %d KB (pagina = %d KB)\n",
           frames, new_ram_kb, mem_state.page_size_kb);
    printf("[INFO] Particionamiento dinamico: 1 bloque libre continuo de %d KB inicializado.\n", new_ram_kb);
    return 0;
}

/**
 * mem-config default
 *
 * Preset seguro del Issue 3.2: 1024 KB RAM, 64 KB/página, 16 marcos, First-Fit.
 * Rechazado si hay procesos activos en memoria.
 */
int memory_config_default(void) {
    if (has_active_processes()) {
        fprintf(stderr,
            "[ERROR] No se puede aplicar el preset: hay procesos activos en memoria.\n"
            "        Libera la memoria con 'mem-free <pid>' antes de reconfigurar.\n");
        return 1;
    }

    const int ram_kb  = 1024;
    const int page_kb = 64;
    const int frames  = ram_kb / page_kb; /* 16 */

    mem_state.total_ram_kb = ram_kb;
    mem_state.page_size_kb = page_kb;
    mem_state.total_frames = frames;
    strcpy(mem_state.algorithm, "ff");
    mem_reset_blocks();

    printf("[OK] Preset aplicado: RAM = %d KB | Pagina = %d KB | Marcos = %d | Algoritmo = First-Fit\n",
           ram_kb, page_kb, frames);
    printf("[INFO] Particionamiento dinamico: 1 bloque libre continuo de %d KB inicializado.\n", ram_kb);
    return 0;
}

/**
 * Helper para imprimir una fila formateada y perfectamente alineada
 * dentro del marco de la tabla de mem-status.
 * Ancho total de la fila: exactamente 52 caracteres (incluyendo bordes).
 */
static void print_status_row(const char *label, const char *value) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%-12s: %s", label, value);
    printf("  |  %-44s  |\n", buf);
}

/**
 * mem-status
 *
 * Calcula KB usados/libres y bloques iterando la lista enlazada.
 * Presenta el resumen adaptado al algoritmo activo con alineación exacta.
 */
void memory_status(void) {
    int used_kb     = 0, free_kb     = 0;
    int used_blocks = 0, free_blocks = 0;

    const MemBlock *b = mem_state.block_list;
    while (b != NULL) {
        if (b->status == BLOCK_USED) { used_kb += b->size_kb; used_blocks++; }
        else                         { free_kb += b->size_kb; free_blocks++; }
        b = b->next;
    }

    /* Porcentaje con protección contra división por cero */
    float pct_used = (mem_state.total_ram_kb > 0)
        ? (100.0f * used_kb / mem_state.total_ram_kb) : 0.0f;
    float pct_free = 100.0f - pct_used;

    const char *algo_name = memory_is_first_fit()
        ? "First-Fit (Particion Dinamica)"
        : "Paginacion (Marcos Fijos)";

    char val_buf[64];

    printf("\n");
    printf("  +================================================+\n");
    printf("  |          ESTADO DE MEMORIA -- MiniOS           |\n");
    printf("  +================================================+\n");

    snprintf(val_buf, sizeof(val_buf), "%d KB", mem_state.total_ram_kb);
    print_status_row("RAM total", val_buf);

    print_status_row("Algoritmo", algo_name);

    if (memory_is_first_fit()) {
        /* Modo FF: información de bloques dinámicos */
        print_status_row("Esquema", "Bloques contiguos variables");

        snprintf(val_buf, sizeof(val_buf), "%d", free_blocks);
        print_status_row("Bloques lib.", val_buf);

        snprintf(val_buf, sizeof(val_buf), "%d", used_blocks);
        print_status_row("Bloques uso.", val_buf);
    } else {
        /* Modo PAG: información de marcos */
        snprintf(val_buf, sizeof(val_buf), "%d KB", mem_state.page_size_kb);
        print_status_row("Tamano pag.", val_buf);

        snprintf(val_buf, sizeof(val_buf), "%d", mem_state.total_frames);
        print_status_row("Marcos tot.", val_buf);

        int used_frames = (mem_state.page_size_kb > 0)
            ? used_kb / mem_state.page_size_kb : 0;
        snprintf(val_buf, sizeof(val_buf), "%d", mem_state.total_frames - used_frames);
        print_status_row("Marcos lib.", val_buf);

        snprintf(val_buf, sizeof(val_buf), "%d", used_frames);
        print_status_row("Marcos uso.", val_buf);
    }

    snprintf(val_buf, sizeof(val_buf), "%d KB (%.1f%%)", free_kb, pct_free);
    print_status_row("Libre", val_buf);

    snprintf(val_buf, sizeof(val_buf), "%d KB (%.1f%%)", used_kb, pct_used);
    print_status_row("Usado", val_buf);

    printf("  +================================================+\n");
    printf("\n");
}

int memory_switch_algorithm(const char *target) {
    if (strcmp(target, "ff") == 0) {
        if (memory_is_first_fit()) {
            printf("[INFO] El algoritmo ya es First-Fit. No se realizaron cambios.\n");
            return 0;
        }

        if (has_active_processes()) {
            char ans[32];
            printf("ADVERTENCIA: Hay proceso(s) en RAM.\n");
            printf("             El cambio a 'ff' liberara toda la memoria.\n");
            printf("¿Desea continuar? [s/N]: ");
            fflush(stdout);
            if (fgets(ans, sizeof(ans), stdin) == NULL || (ans[0] != 's' && ans[0] != 'S')) {
                printf("[CANCELADO] Cambio de algoritmo cancelado por el usuario.\n");
                return 0;
            }
        }

        strcpy(mem_state.algorithm, "ff");
        s_mem_config.active_algorithm = MEM_ALGO_FIRST_FIT;
        mem_reset_blocks();
        printf("[OK] Algoritmo cambiado a: First-Fit\n");
        printf("[INFO] Tablas de paginas y marcos fisicos purgados.\n");
        printf("[INFO] Memoria reinicializada como 1 bloque continuo gigante libre (%d KB).\n",
               mem_state.total_ram_kb);
        return 0;
    }

    if (strcmp(target, "pag") == 0) {
        if (!memory_is_first_fit()) {
            printf("[INFO] El algoritmo ya es Paginacion. No se realizaron cambios.\n");
            return 0;
        }

        if (has_active_processes()) {
            char ans[32];
            printf("ADVERTENCIA: Hay proceso(s) en RAM.\n");
            printf("             El cambio a 'pag' liberara toda la memoria.\n");
            printf("¿Desea continuar? [s/N]: ");
            fflush(stdout);
            if (fgets(ans, sizeof(ans), stdin) == NULL || (ans[0] != 's' && ans[0] != 'S')) {
                printf("[CANCELADO] Cambio de algoritmo cancelado por el usuario.\n");
                return 0;
            }
        } else {
            printf("[INFO] Memoria libre. Cambiando de particionamiento dinamico a paginacion...\n");
        }

        strcpy(mem_state.algorithm, "pag");
        s_mem_config.active_algorithm = MEM_ALGO_PAGING;
        mem_reset_blocks();
        printf("[OK] Algoritmo cambiado a: Paginacion\n");
        printf("[INFO] Lista de bloques First-Fit limpiada.\n");
        printf("[INFO] Memoria fisica estructurada en %d marcos fijos de %d KB.\n",
               mem_state.total_frames, mem_state.page_size_kb);
        return 0;
    }

    return 1;
}
