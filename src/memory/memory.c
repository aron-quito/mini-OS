/**
 * @file memory.c
 * @brief Implementación del subsistema de memoria física simulada.
 *
 * Issue 3.1 (SIS-46): Modela la RAM como una lista enlazada ordenada por start_kb,
 * arrancando con un único bloque gigante libre que cubre toda la memoria.
 * Issue 4.2 (SIS-51): Encolado automático en mem_queue cuando ff_alloc falla
 * por memoria insuficiente o fragmentación externa.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "paging.h"
#include "mem_queue.h"  /* Issue 4.2 (SIS-51): encolado reactivo */

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
    if (s_mem_config.active_algorithm == MEM_ALGO_PAGING) {
        strcpy(mem_state.algorithm, "pag");
    } else {
        strcpy(mem_state.algorithm, "ff");
    }
    mem_state.page_size_kb = s_mem_config.page_size_kb;
    mem_state.total_frames = s_mem_config.total_frames;

    if (mem_state.total_ram_kb <= 0) {
        mem_state.total_ram_kb = s_mem_config.total_ram_kb;
    }
    if (mem_state.page_size_kb > 0) {
        mem_state.total_frames = mem_state.total_ram_kb / mem_state.page_size_kb;
    }

    // Inicializa la lista enlazada con el bloque gigante libre
    mem_reset_blocks();

    // Inicializa las estructuras de paginación (Issue 5.1)
    paging_init(mem_state.total_ram_kb, mem_state.page_size_kb);

    /* Issue 4.2: inicializar la cola de espera junto con la memoria */
    mem_queue_init();
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

/**
 * @brief Asigna memoria a un proceso. Si falla, encola automáticamente (Issue 4.2).
 *
 * Flujo:
 *   1. Intenta asignar con ff_alloc (o pag_alloc en Milestone 5).
 *   2. Si la asignación falla (-1): muestra alerta y encola el proceso en mem_queue.
 *   3. Si la asignación tiene éxito (0): retorna 0 normalmente.
 *
 * @param pid       PID del proceso solicitante.
 * @param size_kb   Kilobytes de RAM solicitados.
 * @param command   Nombre del comando (para registrar en la cola).
 * @return 0 si se asignó exitosamente, 1 si fue encolado, -1 en error grave.
 */
int memory_alloc(int pid, int size_kb) {
    /* Validacion: rechazar si el proceso pide mas RAM de la que existe en total */
    if (size_kb > mem_state.total_ram_kb) {
        fprintf(stderr,
            "[ERROR] PID %d solicita %d KB pero la RAM total del sistema es %d KB.\n"
            "        Solicitud rechazada (nunca cabria en memoria).\n",
            pid, size_kb, mem_state.total_ram_kb);
        return -1;
    }

    int result;

    if (memory_is_first_fit()) {
        result = ff_alloc(pid, size_kb);
    } else {
        result = pag_alloc(pid, size_kb);
    }

    if (result == -1) {
        if (memory_is_first_fit()) {
            /* Memoria insuficiente o fragmentación — encolar automáticamente */
            printf("[ALERTA] Memoria RAM insuficiente para asignar %d KB al PID %d.\n",
                   size_kb, pid);
            int pos = mem_queue_enqueue(pid, size_kb, "");
            if (pos < 0) {
                fprintf(stderr, "[ERROR] No se pudo encolar el proceso PID %d (malloc fallido).\n", pid);
                return -1;
            }
            printf("[QUEUE] Proceso PID %d encolado en cola de espera (Posicion: %d).\n",
                   pid, pos);
        }
        return 1;  /* Encolado exitoso */
    }

    return 0;  /* Asignado exitosamente */
}


int memory_free(int pid) {
    if (memory_is_first_fit()) {
        return ff_free(pid);
    }
    return paging_free(pid);
}

/**
 * @brief Reasigna el PID de un bloque de memoria de old_pid a new_pid.
 *
 * Se invoca en process_execute_bg() justo después del fork() para corregir
 * el PID provisional (0) al PID real del proceso hijo recién creado.
 * Sin esta llamada, memory_free(real_pid) no encontraría el bloque y la RAM
 * quedaría marcada como BLOCK_USED para siempre (memory leak simulado).
 *
 * @param old_pid  PID provisional con que se reservó (normalmente 0).
 * @param new_pid  PID real del proceso hijo obtenido del fork().
 * @return 0 si el bloque fue encontrado y actualizado, -1 si no existe.
 */
int memory_reassign_pid(int old_pid, int new_pid) {
    MemBlock *b = mem_state.block_list;
    while (b != NULL) {
        if (b->status == BLOCK_USED && b->process_id == old_pid) {
            b->process_id = new_pid;
            if (memory_is_first_fit()) {
                printf("[FF] PID %d asignado en dirección %d KB (%d KB)\n",
                       new_pid, b->start_kb, b->size_kb);
            }
            return 0;
        }
        b = b->next;
    }

    /* En modo Paginación: actualizar la page_table del proceso */
    if (!memory_is_first_fit()) {
        pag_free(old_pid); /* Libera la entrada antigua */
    }

    return -1; /* Bloque no encontrado */
}

void memory_view(void) {
    if (memory_is_first_fit()) {
        render_mem_view_ff();
    } else {
        render_mem_view_pag();
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
    mem_queue_clear();  /* Issue 4.2: limpiar cola de espera también */
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
    if (frame_table.total_frames > 0 && frame_table.free_frames < frame_table.total_frames) {
        return 1;
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
    paging_init(mem_state.total_ram_kb, mem_state.page_size_kb);

    printf("[OK] RAM total configurada: %d KB\n", ram_kb);
    printf("[INFO] Particionamiento dinamico: 1 bloque libre continuo de %d KB inicializado.\n", ram_kb);
    printf("[INFO] Marcos recalculados: %d (pagina = %d KB)\n",
           mem_state.total_frames, mem_state.page_size_kb);

    /* Fix #2: Intentar asignar procesos en cola que ahora puedan caber */
    if (mem_queue_length() > 0) {
        printf("[QUEUE] RAM ampliada — revisando cola de espera...\n");
        mem_queue_process_waiting();
    }
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
            "[ERROR] %d KB no es potencia de 2. Valores validos: 4, 8, 16, 32, 64, 128, 256...\n",
            page_kb);
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
    paging_init(mem_state.total_ram_kb, mem_state.page_size_kb);

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
    paging_init(mem_state.total_ram_kb, mem_state.page_size_kb);

    printf("[OK] Marcos configurados: %d | RAM recalculada: %d KB (pagina = %d KB)\n",
           frames, new_ram_kb, mem_state.page_size_kb);
    printf("[INFO] Particionamiento dinamico: 1 bloque libre continuo de %d KB inicializado.\n", new_ram_kb);

    /* Fix #2: Intentar asignar procesos en cola que ahora puedan caber */
    if (mem_queue_length() > 0) {
        printf("[QUEUE] RAM ampliada — revisando cola de espera...\n");
        mem_queue_process_waiting();
    }
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
    paging_init(mem_state.total_ram_kb, mem_state.page_size_kb);

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

    if (!memory_is_first_fit() && frame_table.total_frames > 0) {
        int used_frames = frame_table.total_frames - frame_table.free_frames;
        used_kb = used_frames * mem_state.page_size_kb;
        free_kb = frame_table.free_frames * mem_state.page_size_kb;
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

/* =========================================================================
 * Issue 6.1 (SIS-57): Visualizador gráfico interactivo de RAM (mem-view / ram-view)
 * ========================================================================= */

#define MEM_VIEW_INNER_WIDTH 66  /* Columnas visibles después de "║  " hasta "║" */

/**
 * @brief Calcula la longitud en caracteres visibles (code points UTF-8) de una cadena.
 */
static int utf8_visible_len(const char *s) {
    int count = 0;
    while (*s != '\0') {
        if (((unsigned char)*s & 0xC0) != 0x80) {
            count++;
        }
        s++;
    }
    return count;
}

/**
 * @brief Imprime una fila dentro del recuadro Unicode de 68 columnas internas ("║  ...║")
 *        calculando el relleno exacto según la cantidad de caracteres UTF-8 visibles.
 */
static void print_view_box_line(const char *text) {
    int vis = utf8_visible_len(text);
    int pad = MEM_VIEW_INNER_WIDTH - vis;
    if (pad < 0) {
        pad = 0;
    }
    printf("║  %s%*s║\n", text, pad, "");
}

/**
 * @brief Formatea la línea de estado de la cola de espera (mem_queue) para el pie del mapa.
 */
static void format_queue_summary_line(char *out_buf, size_t buf_size) {
    int q_len = mem_queue_length();
    if (q_len <= 0) {
        snprintf(out_buf, buf_size, "Cola de Espera RAM: Vacía (0 procesos esperando)");
    } else {
        mem_queue_node_t *front = mem_queue_peek();
        if (front != NULL) {
            snprintf(out_buf, buf_size,
                     "Cola de Espera RAM: %d proceso(s) en espera (PID %d: %d KB)",
                     q_len, front->pid, front->size_kb);
        } else {
            snprintf(out_buf, buf_size,
                     "Cola de Espera RAM: %d proceso(s) en espera", q_len);
        }
    }
}

/**
 * @brief Renderiza el mapa de memoria en modo First-Fit (Issue 6.1).
 */
void render_mem_view_ff(void) {
    int used_kb     = 0;
    int free_kb     = 0;
    int free_blocks = 0;

    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║                      MAPA DE MEMORIA RAM (MiniOS)                  ║\n");
    printf("║                      Algoritmo Activo: FIRST-FIT                   ║\n");
    printf("╠════════════════════════════════════════════════════════════════════╣\n");

    char row_buf[256] = "";
    const MemBlock *curr = mem_state.block_list;

    while (curr != NULL) {
        char token[64];
        if (curr->status == BLOCK_USED) {
            used_kb += curr->size_kb;
            snprintf(token, sizeof(token), "[P%d: %dK | USED]", curr->process_id, curr->size_kb);
        } else {
            free_kb += curr->size_kb;
            free_blocks++;
            snprintf(token, sizeof(token), "[FREE: %dK]", curr->size_kb);
        }

        size_t cur_len = strlen(row_buf);
        size_t tok_len = strlen(token);
        /* Si agregar el token supera el ancho interno útil (64 cols), volcar línea actual */
        if (cur_len > 0 && (cur_len + 1 + tok_len) > 64) {
            print_view_box_line(row_buf);
            row_buf[0] = '\0';
            cur_len = 0;
        }

        if (cur_len > 0) {
            strncat(row_buf, " ", sizeof(row_buf) - strlen(row_buf) - 1);
        }
        strncat(row_buf, token, sizeof(row_buf) - strlen(row_buf) - 1);

        curr = curr->next;
    }

    if (row_buf[0] != '\0') {
        print_view_box_line(row_buf);
    } else {
        print_view_box_line("[SIN BLOQUES]");
    }

    printf("╠════════════════════════════════════════════════════════════════════╣\n");

    float pct_used = (mem_state.total_ram_kb > 0)
        ? (100.0f * (float)used_kb / (float)mem_state.total_ram_kb) : 0.0f;
    float pct_free = (mem_state.total_ram_kb > 0)
        ? (100.0f - pct_used) : 0.0f;

    char line_buf[256];
    snprintf(line_buf, sizeof(line_buf),
             "Total: %d KB | Usado: %d KB (%.1f%%) | Libre: %d KB (%.1f%%)",
             mem_state.total_ram_kb, used_kb, pct_used, free_kb, pct_free);
    print_view_box_line(line_buf);

    snprintf(line_buf, sizeof(line_buf),
             "Fragmentación Externa: %d bloques libres no contiguos (%d KB)",
             free_blocks, free_kb);
    print_view_box_line(line_buf);

    format_queue_summary_line(line_buf, sizeof(line_buf));
    print_view_box_line(line_buf);

    printf("╚════════════════════════════════════════════════════════════════════╝\n");
}

/**
 * @brief Renderiza el mapa de memoria en modo Paginación (Issue 6.1).
 */
void render_mem_view_pag(void) {
    int page_kb = (mem_state.page_size_kb > 0) ? mem_state.page_size_kb : DEFAULT_PAGE_KB;
    if (frame_table.total_frames <= 0 && mem_state.total_ram_kb > 0) {
        paging_init(mem_state.total_ram_kb, page_kb);
    }

    int total_frames = frame_table.total_frames;
    int free_frames  = frame_table.free_frames;
    int used_frames  = total_frames - free_frames;
    int used_kb      = used_frames * page_kb;
    int free_kb      = free_frames * page_kb;

    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║                      MAPA DE MEMORIA RAM (MiniOS)                  ║\n");
    printf("║                      Algoritmo Activo: PAGINACIÓN                  ║\n");
    printf("╠════════════════════════════════════════════════════════════════════╣\n");

    char line_buf[256];
    snprintf(line_buf, sizeof(line_buf),
             "Marcos Físicos (Total: %d marcos de %d KB c/u):",
             total_frames, page_kb);
    print_view_box_line(line_buf);

    /* Imprimir cuadrícula de marcos físicos (4 marcos por fila) */
    for (int f = 0; f < total_frames; f += 4) {
        char grid_row[256] = "";
        for (int col = 0; col < 4 && (f + col) < total_frames; col++) {
            int idx   = f + col;
            int owner = frame_table.frame_owner[idx];
            char cell[32];

            if (owner == -1) {
                snprintf(cell, sizeof(cell), "[M%02d: LIBRE]", idx);
            } else {
                snprintf(cell, sizeof(cell), "[M%02d: P%d]", idx, owner);
            }

            if (col > 0) {
                strncat(grid_row, " ", sizeof(grid_row) - strlen(grid_row) - 1);
            }
            strncat(grid_row, cell, sizeof(grid_row) - strlen(grid_row) - 1);
        }
        print_view_box_line(grid_row);
    }

    printf("╠════════════════════════════════════════════════════════════════════╣\n");

    snprintf(line_buf, sizeof(line_buf),
             "Total: %d KB | Usado: %d KB (%d marcos) | Libre: %d KB (%d m)",
             mem_state.total_ram_kb, used_kb, used_frames, free_kb, free_frames);
    print_view_box_line(line_buf);

    format_queue_summary_line(line_buf, sizeof(line_buf));
    print_view_box_line(line_buf);

    printf("╚════════════════════════════════════════════════════════════════════╝\n");
}

/**
 * @brief Comando principal para mem-view y su alias ram-view (Issue 6.1).
 */
int cmd_mem_view(int argc, char **argv) {
    (void)argc;
    (void)argv;
    memory_view();
    return 0;
}
