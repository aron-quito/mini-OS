#include <stdio.h>
#include "shell/shell.h"

int main(void) {
    printf("================================================================\n");
    printf("         miniOS — Shell Interactivo Educativo en C11            \n");
    printf("================================================================\n\n");

    shell_init();
    shell_loop();

    return 0;
}
