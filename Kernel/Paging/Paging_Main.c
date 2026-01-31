#include "Paging_Main.h"

extern void paging_init_asm(void);  // ASM 本体

void init_paging(void) {
    paging_init_asm();  // 呼び出す
}
