#include "Memory_Main.h"
#include "../Kernel_Main.h"
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 262144

static uint8_t page_bitmap[MAX_PAGES];

typedef struct memory_block {
    uint32_t size;
    int is_free;
    struct memory_block* next;
} memory_block_t;

static memory_block_t* heap_start = NULL;
static uint32_t heap_initialized = 0;
static uint32_t total_allocated = 0;
static uint32_t total_freed = 0;

#define HEAP_START_PAGE 256
#define HEAP_PAGE_COUNT 4096

void init_physical_memory(void *memory_map, size_t map_size, size_t desc_size) {
    serial_write_string("[OS] [Memory] Start Initalize Physical Memory.\n");
    (void)memory_map;
    (void)map_size;
    (void)desc_size;
    
    for (size_t i = 0; i < MAX_PAGES; i++)
        page_bitmap[i] = 0;
    
    for (size_t i = HEAP_START_PAGE; i < HEAP_START_PAGE + HEAP_PAGE_COUNT; i++) {
        page_bitmap[i] = 1;
    }
    serial_write_string("[OS] [Memory] Success Initalize Physical Memory.\n");
}

void memory_init(void) {
    if (heap_initialized) return;
    
    heap_start = (memory_block_t*)(HEAP_START_PAGE * PAGE_SIZE);
    
    heap_start->size = (HEAP_PAGE_COUNT * PAGE_SIZE) - sizeof(memory_block_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;
    
    heap_initialized = 1;
    total_allocated = 0;
    total_freed = 0;
}

void* kmalloc(uint32_t size) {
    if (!heap_initialized) {
        memory_init();
    }
    
    if (size == 0) return NULL;
    
    size = (size + 7) & ~7;
    
    memory_block_t* current = heap_start;
    
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            if (current->size >= size + sizeof(memory_block_t) + 64) {
                memory_block_t* new_block = (memory_block_t*)((uint8_t*)current + sizeof(memory_block_t) + size);
                new_block->size = current->size - size - sizeof(memory_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            total_allocated += current->size;
            
            return (void*)((uint8_t*)current + sizeof(memory_block_t));
        }
        
        current = current->next;
    }
    
    return NULL;
}

void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t heap_start_addr = (uintptr_t)heap_start;
    uintptr_t heap_end_addr = heap_start_addr + (HEAP_PAGE_COUNT * PAGE_SIZE);
    
    if (addr < heap_start_addr || addr >= heap_end_addr) {
        return;
    }
    
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    
    if (block->is_free) return;
    
    block->is_free = 1;
    total_freed += block->size;
    
    memory_block_t* current = heap_start;
    
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(memory_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void* kcalloc(uint32_t num, uint32_t size) {
    uint32_t total_size = num * size;
    void* ptr = kmalloc(total_size);
    
    if (ptr != NULL) {
        uint8_t* byte_ptr = (uint8_t*)ptr;
        for (uint32_t i = 0; i < total_size; i++) {
            byte_ptr[i] = 0;
        }
    }
    
    return ptr;
}

void* krealloc(void* ptr, uint32_t new_size) {
    if (ptr == NULL) {
        return kmalloc(new_size);
    }
    
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    uint32_t old_size = block->size;
    
    if (new_size <= old_size) {
        return ptr;
    }
    
    void* new_ptr = kmalloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    uint8_t* src = (uint8_t*)ptr;
    uint8_t* dst = (uint8_t*)new_ptr;
    for (uint32_t i = 0; i < old_size; i++) {
        dst[i] = src[i];
    }
    
    kfree(ptr);
    
    return new_ptr;
}

uint32_t get_free_memory(void) {
    if (!heap_initialized) return 0;
    
    uint32_t free_memory = 0;
    memory_block_t* current = heap_start;
    
    while (current != NULL) {
        if (current->is_free) {
            free_memory += current->size;
        }
        current = current->next;
    }
    
    return free_memory;
}

uint32_t get_used_memory(void) {
    return total_allocated - total_freed;
}

void debug_print_memory_info(void) {
    if (!heap_initialized) {
        return;
    }
    
    memory_block_t* current = heap_start;
    int block_count = 0;
    
    while (current != NULL) {
        block_count++;
        current = current->next;
    }
}

void* alloc_page(void) {
    for (size_t i = 0; i < MAX_PAGES; i++) {
        if (page_bitmap[i] == 0) {
            page_bitmap[i] = 1;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void free_page(void* addr) {
    uintptr_t page_num = (uintptr_t)addr / PAGE_SIZE;
    if (page_num < MAX_PAGES) {
        page_bitmap[page_num] = 0;
    }
}
