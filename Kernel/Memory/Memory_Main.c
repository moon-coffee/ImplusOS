#include "Memory_Main.h"
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 262144

static uint8_t page_bitmap[MAX_PAGES];

// ヒープ管理用の構造体
typedef struct memory_block {
    uint32_t size;
    int is_free;
    struct memory_block* next;
} memory_block_t;

// ヒープの開始アドレスと管理変数
static memory_block_t* heap_start = NULL;
static uint32_t heap_initialized = 0;
static uint32_t total_allocated = 0;
static uint32_t total_freed = 0;

// ヒープ領域の定義（適宜調整してください）
#define HEAP_START_PAGE 256      // 1MBから開始（256 * 4096）
#define HEAP_PAGE_COUNT 4096     // 16MBのヒープ（4096 * 4096）

void init_physical_memory(void *memory_map, size_t map_size, size_t desc_size) {
    (void)memory_map;
    (void)map_size;
    (void)desc_size;
    
    // ページビットマップ初期化
    for (size_t i = 0; i < MAX_PAGES; i++)
        page_bitmap[i] = 0;
    
    // ヒープ用のページを確保
    for (size_t i = HEAP_START_PAGE; i < HEAP_START_PAGE + HEAP_PAGE_COUNT; i++) {
        page_bitmap[i] = 1; // 使用中としてマーク
    }
}

// メモリ管理システムの初期化
void memory_init(void) {
    if (heap_initialized) return;
    
    // ヒープ開始アドレスを計算
    heap_start = (memory_block_t*)(HEAP_START_PAGE * PAGE_SIZE);
    
    // 最初の大きなフリーブロックを作成
    heap_start->size = (HEAP_PAGE_COUNT * PAGE_SIZE) - sizeof(memory_block_t);
    heap_start->is_free = 1;
    heap_start->next = NULL;
    
    heap_initialized = 1;
    total_allocated = 0;
    total_freed = 0;
}

// メモリ割り当て
void* kmalloc(uint32_t size) {
    if (!heap_initialized) {
        memory_init();
    }
    
    if (size == 0) return NULL;
    
    // 8バイト境界にアラインメント
    size = (size + 7) & ~7;
    
    memory_block_t* current = heap_start;
    
    // First-fit アルゴリズム
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            // 十分な大きさの空きブロックを見つけた
            
            // ブロックを分割するか判断（残りが64バイト以上ある場合のみ分割）
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
            
            // データ領域のポインタを返す（ヘッダの後ろ）
            return (void*)((uint8_t*)current + sizeof(memory_block_t));
        }
        
        current = current->next;
    }
    
    // メモリ不足
    return NULL;
}

// メモリ解放
void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    // ヒープ範囲外のアドレスをチェック
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t heap_start_addr = (uintptr_t)heap_start;
    uintptr_t heap_end_addr = heap_start_addr + (HEAP_PAGE_COUNT * PAGE_SIZE);
    
    if (addr < heap_start_addr || addr >= heap_end_addr) {
        return; // 無効なポインタ
    }
    
    // ヘッダ位置を取得
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    
    // すでに解放済みの場合は何もしない（二重解放防止）
    if (block->is_free) return;
    
    block->is_free = 1;
    total_freed += block->size;
    
    // 隣接する空きブロックと結合（断片化を減らす）
    memory_block_t* current = heap_start;
    
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            // 2つの連続した空きブロックを結合
            current->size += sizeof(memory_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

// ゼロクリアして割り当て
void* kcalloc(uint32_t num, uint32_t size) {
    uint32_t total_size = num * size;
    void* ptr = kmalloc(total_size);
    
    if (ptr != NULL) {
        // ゼロクリア
        uint8_t* byte_ptr = (uint8_t*)ptr;
        for (uint32_t i = 0; i < total_size; i++) {
            byte_ptr[i] = 0;
        }
    }
    
    return ptr;
}

// メモリ再割り当て
void* krealloc(void* ptr, uint32_t new_size) {
    if (ptr == NULL) {
        return kmalloc(new_size);
    }
    
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    // 現在のブロックサイズを取得
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    uint32_t old_size = block->size;
    
    // 新しいサイズが現在のサイズより小さいか同じ場合
    if (new_size <= old_size) {
        return ptr; // そのまま返す
    }
    
    // 新しいメモリを割り当て
    void* new_ptr = kmalloc(new_size);
    if (new_ptr == NULL) {
        return NULL; // 割り当て失敗
    }
    
    // データをコピー
    uint8_t* src = (uint8_t*)ptr;
    uint8_t* dst = (uint8_t*)new_ptr;
    for (uint32_t i = 0; i < old_size; i++) {
        dst[i] = src[i];
    }
    
    // 古いメモリを解放
    kfree(ptr);
    
    return new_ptr;
}

// 空きメモリ量を取得
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

// 使用中メモリ量を取得
uint32_t get_used_memory(void) {
    return total_allocated - total_freed;
}

// デバッグ用：メモリ状態を出力
void debug_print_memory_info(void) {
    if (!heap_initialized) {
        // 初期化されていない場合の処理
        return;
    }
    
    memory_block_t* current = heap_start;
    int block_count = 0;
    
    while (current != NULL) {
        // ここでログ出力などを行う（実装依存）
        block_count++;
        current = current->next;
    }
}

// ページ単位でのメモリ割り当て（必要に応じて）
void* alloc_page(void) {
    for (size_t i = 0; i < MAX_PAGES; i++) {
        if (page_bitmap[i] == 0) {
            page_bitmap[i] = 1;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL; // ページ不足
}

// ページ単位でのメモリ解放
void free_page(void* addr) {
    uintptr_t page_num = (uintptr_t)addr / PAGE_SIZE;
    if (page_num < MAX_PAGES) {
        page_bitmap[page_num] = 0;
    }
}