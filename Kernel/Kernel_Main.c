#include "Kernel_Main.h"
#include "Memory/Memory_Main.h"
#include "Paging/Paging_Main.h"
#include "IDT/IDT_Main.h"
#include "IO/IO_Main.h"
#include "Drivers/FileSystem/FAT32/FAT32_Main.h"

#define COM1_PORT 0x3F8

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    // Disable interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB
    outb(COM1_PORT + 0, 0x03);    // Divisor low  (38400 baud)
    outb(COM1_PORT + 1, 0x00);    // Divisor high
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void serial_write_char(char c) {
    while ((inb(COM1_PORT + 5) & 0x20) == 0); // Wait for TX empty
    outb(COM1_PORT, c);
}

void serial_write_string(const char* str) {
    while (*str) {
        if (*str == '\n')
            serial_write_char('\r');
        serial_write_char(*str++);
    }
}

void serial_write_uint64(uint64_t value) {
    char hex[] = "0123456789ABCDEF";
    serial_write_string("0x");

    for (int i = 60; i >= 0; i -= 4) {
        serial_write_char(hex[(value >> i) & 0xF]);
    }
}

void serial_write_uint32(uint32_t value) {
    serial_write_uint64((uint64_t)value);
}

void serial_write_uint16(uint16_t value) {
    serial_write_uint64((uint64_t)value);
}

__attribute__((noreturn))
void kernel_main(BOOT_INFO *boot_info){
    fat32_list_root_files();
    init_physical_memory(
        boot_info->MemoryMap,
        boot_info->MemoryMapSize,
        boot_info->MemoryMapDescriptorSize
    );
    memory_init();
    init_paging();
    init_idt();
    
    int fat32_ok = 0;
    if(fat32_init()){
        fat32_ok = 1;
    }

    while(1) {
        __asm__("hlt");
    }
}