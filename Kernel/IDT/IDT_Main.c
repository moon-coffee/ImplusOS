#include "IDT_Main.h"
#include "../IO/IO_Main.h"
#include "../Serial.h"

#define MAX_IRQS 256

static IDT_Entry idt[IDT_ENTRIES];
static IDT_Ptr idt_ptr;
static isr_t irq_routines[MAX_IRQS] = {0};

extern void isr_default(void);
extern void isr_page_fault(void);
extern void load_idt(IDT_Ptr* idt_ptr);

void register_interrupt_handler(uint16_t irq, isr_t handler) {
    if (irq < MAX_IRQS) {
        irq_routines[irq] = handler;
    }
}

void irq_handler(uint16_t irq_num) {
    if (irq_num < MAX_IRQS && irq_routines[irq_num]) {
        irq_routines[irq_num]();
    }
    
    if (irq_num >= 32 && irq_num < 48) {
        outb(0x20, 0x20);
        if (irq_num >= 40) {
            outb(0xA0, 0x20);
        }
    }
}

void set_interrupt_handler(uint16_t n, void (*handler)(void)) {
    uint64_t addr = (uint64_t)handler;
    
    idt[n].offset_low   = addr & 0xFFFF;
    idt[n].selector     = 0x08;
    idt[n].ist          = 0;
    idt[n].type_attr    = 0x8E;
    idt[n].offset_mid   = (addr >> 16) & 0xFFFF;
    idt[n].offset_high  = (addr >> 32) & 0xFFFFFFFF;
    idt[n].zero         = 0;
}

void init_idt(void) {
    serial_write_string("[OS] [IDT] Start Initialize IDT.\n");
    
    for (int i = 0; i < IDT_ENTRIES; i++) {
        set_interrupt_handler(i, isr_default);
    }

    set_interrupt_handler(14, isr_page_fault);
    
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint64_t)&idt;
    
    load_idt(&idt_ptr);
    
    serial_write_string("[OS] [IDT] Successfully Initialize IDT.\n");
}

void page_fault_handler(uint64_t error_code, uint64_t rip, uint64_t rsp, uint64_t cr2) {
    serial_write_string("[OS] [PF] Page fault\n");
    serial_write_string("[OS] [PF] CR2: ");
    serial_write_uint64(cr2);
    serial_write_string("\n");
    serial_write_string("[OS] [PF] RIP: ");
    serial_write_uint64(rip);
    serial_write_string("\n");
    serial_write_string("[OS] [PF] RSP: ");
    serial_write_uint64(rsp);
    serial_write_string("\n");
    serial_write_string("[OS] [PF] Error: ");
    serial_write_uint64(error_code);
    serial_write_string("\n");

    while (1) {
        __asm__("hlt");
    }
}

void unregister_interrupt_handler(uint16_t irq) {
    if (irq < MAX_IRQS) {
        irq_routines[irq] = 0;
    }
}

void set_exception_handler(uint16_t exception_num, void (*handler)(void)) {
    if (exception_num < 32) {
        set_interrupt_handler(exception_num, handler);
    }
}

void set_irq_handler(uint16_t irq, void (*handler)(void)) {
    if (irq < 16) {
        set_interrupt_handler(32 + irq, handler);
    }
}
