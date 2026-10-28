#include <stdio.h>
#include <string.h>
#include "memory/paging.h"

static frame_t s_frames[MAX_FRAMES];
static int s_total_frames = 0;
static int s_page_size = 4;

void paging_init(int total_ram_kb, int page_size_kb) {
    s_page_size = (page_size_kb > 0) ? page_size_kb : 4;
    s_total_frames = total_ram_kb / s_page_size;
    if (s_total_frames > MAX_FRAMES) s_total_frames = MAX_FRAMES;

    for (int i = 0; i < s_total_frames; i++) {
        s_frames[i].frame_number = i;
        s_frames[i].is_free = 1;
        s_frames[i].pid = 0;
    }
}

int paging_alloc(int pid, int size_kb) {
    (void)pid;
    (void)size_kb;
    // Stub inicial para Issue 1.1 (se implementará en Issue 5.1)
    return 0;
}

int paging_free(int pid) {
    (void)pid;
    // Stub inicial para Issue 1.1 (se implementará en Issue 5.3)
    return 0;
}

int paging_translate(int pid, int logical_address, int *out_physical_address) {
    (void)pid;
    (void)logical_address;
    if (out_physical_address) *out_physical_address = 0;
    // Stub inicial para Issue 1.1 (se implementará en Issue 5.2)
    return 0;
}

void paging_view(void) {
    printf("  [Paginación]: %d marcos físicos configurados de %d KB cada uno.\n",
           s_total_frames, s_page_size);
}

void paging_cleanup(void) {
    s_total_frames = 0;
}
