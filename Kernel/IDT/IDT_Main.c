#include "IDT_Main.h"
#define MAX_IRQS 256
static IDT_Entry idt[IDT_ENTRIES];
static IDT_Ptr idt_ptr;

static void default_handler(void){
    while(1) __asm__("hlt");
}

static isr_t irq_routines[MAX_IRQS] = {0};

void register_interrupt_handler(uint8_t irq, isr_t handler){
    irq_routines[irq] = handler;
}

// IRQディスパッチ関数
void irq_handler(uint8_t irq_num){
    if(irq_routines[irq_num]){
        irq_routines[irq_num]();
    }
    // PIC EOI送信など必要
}


void set_interrupt_handler(uint8_t n, void (*handler)(void)){
    uint64_t addr = (uint64_t)handler;
    idt[n].offset_low  = addr & 0xFFFF;
    idt[n].selector    = 0x08;
    idt[n].ist         = 0;
    idt[n].type_attr   = 0x8E;
    idt[n].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[n].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[n].zero        = 0;
}

void init_idt(void){
    for(int i=0;i<IDT_ENTRIES;i++)
        set_interrupt_handler(i,default_handler);

    idt_ptr.limit = sizeof(idt)-1;
    idt_ptr.base  = (uint64_t)&idt;

    load_idt(&idt_ptr);
}
