#include "paging.h"

void paging_init(int total_ram_kb, int page_size_kb) {
    (void)total_ram_kb;
    (void)page_size_kb;
    // Stub base — Se implementará en Milestone 5 (SIS-54)
}

int paging_alloc(int pid, int size_kb) {
    (void)pid;
    (void)size_kb;
    return 0;
}

int paging_free(int pid) {
    (void)pid;
    return 0;
}

int paging_translate(int pid, int logical_address, int *out_physical_address) {
    (void)pid;
    (void)logical_address;
    if (out_physical_address) {
        *out_physical_address = 0;
    }
    return 0;
}

void paging_view(void) {
}

void paging_cleanup(void) {
}
