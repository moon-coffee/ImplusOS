#include "Paging_Main.h"
#include "../Kernel_Main.h"

extern void paging_init_asm(void);

void init_paging(void) {
    serial_write_string("[OS] [Memory] Start Initialize Paging.\n");
    paging_init_asm();
    serial_write_string("[OS] [Memory] Success Initialize Paging.\n");
}
