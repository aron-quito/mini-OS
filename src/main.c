#include "memory/memory.h"
#include "shell/shell.h"
#include <stdio.h>

int main(void) {
  printf("================================================================\n");
  printf("         miniOS — Shell Interactivo Educativo en C11            \n");
  printf(
      "================================================================\n\n");

  // Inicializar sub-sistema de memoria
  memory_init(128, MEM_ALGO_FIRST_FIT);

  // Verificación rápida del bloque libre
  if (mem_state.block_list != NULL) {
    printf("[DEBUG Issue 3.1] RAM: %d KB | Bloque 0: %d KB a %d KB (%s)\n\n",
           mem_state.total_ram_kb, mem_state.block_list->start_kb,
           mem_state.block_list->size_kb,
           mem_state.block_list->status == BLOCK_FREE ? "LIBRE" : "OCUPADO");
  }

  shell_init();
  shell_loop();

  return 0;
}
