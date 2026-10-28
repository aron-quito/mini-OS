#include <stdio.h>
#include <string.h>
#include "process/process.h"

static bg_process_t s_bg_table[MAX_BG_PROCESSES];
static int s_bg_count = 0;

void process_init(void) {
    s_bg_count = 0;
    memset(s_bg_table, 0, sizeof(s_bg_table));
}

int process_execute_fg(char **argv) {
    if (!argv || !argv[0]) return -1;
    // Stub inicial para Issue 1.1
    printf("[PROCESS] Ejecución foreground de '%s' (pendiente implementación completa en Issue 2.1)\n", argv[0]);
    return 0;
}

int process_execute_bg(char **argv, int mem_kb) {
    if (!argv || !argv[0]) return -1;
    // Stub inicial para Issue 1.1
    printf("[PROCESS] Ejecución background de '%s' (%d KB) (pendiente implementación completa en Issue 2.2)\n",
           argv[0], mem_kb);
    return 0;
}

void process_list_mini(void) {
    printf("\n=== PROCESOS DE LA SESIÓN (ps-mini) ===\n");
    printf("%-8s %-8s %-12s %-10s %s\n", "PID", "PPID", "ESTADO", "RAM(KB)", "COMANDO");
    if (s_bg_count == 0) {
        printf("(No hay procesos en segundo plano activos)\n");
    }
    printf("========================================\n\n");
}
