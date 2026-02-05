#ifndef MEMORY_MAIN_H
#define MEMORY_MAIN_H

#include <stdint.h>
#include <stddef.h>

// メモリ管理関数
void* kmalloc(uint32_t size);
void kfree(void* ptr);
void* kcalloc(uint32_t num, uint32_t size);
void* krealloc(void* ptr, uint32_t new_size);

// メモリ初期化
void memory_init(void);
void init_physical_memory(void *memory_map, size_t map_size, size_t desc_size);

// 物理ページ管理
void* alloc_page(void);
void free_page(void* addr);

// メモリ情報取得
uint32_t get_free_memory(void);
uint32_t get_used_memory(void);

#endif // MEMORY_MAIN_H
