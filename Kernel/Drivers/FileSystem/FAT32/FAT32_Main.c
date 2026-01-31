// FAT32_Main.c (修正版)
#include "FAT32_Main.h"
#include <string.h>

static FAT32_BPB bpb;

// --- ヘルパー関数 ---
static uint32_t fat_start_lba() {
    return bpb.reserved_sectors;
}
static uint32_t data_start_lba() {
    return bpb.reserved_sectors + bpb.num_fats * bpb.fat_size_sectors;
}
static uint32_t cluster_to_lba(uint32_t cluster) {
    // クラスタ番号の妥当性チェック
    if(cluster < 2) return 0;
    return data_start_lba() + (cluster - 2) * bpb.sectors_per_cluster;
}

// --- FAT操作 ---
static uint32_t fat_get_next_cluster(uint32_t cluster) {
    // 無効なクラスタ番号のチェック
    if(cluster < 2 || cluster >= 0x0FFFFFF8) return 0x0FFFFFFF;
    
    uint32_t fat_offset = cluster * 4;
    uint32_t sector = fat_start_lba() + (fat_offset / bpb.bytes_per_sector);
    uint32_t offset = fat_offset % bpb.bytes_per_sector;
    
    // オフセットの境界チェック
    if(offset > bpb.bytes_per_sector - 4) return 0x0FFFFFFF;
    
    uint8_t buf[512];
    if(!disk_read(sector, buf, 1)) return 0x0FFFFFFF;
    
    return (*(uint32_t*)&buf[offset]) & 0x0FFFFFFF;
}

uint32_t fat32_get_file_size(FAT32_FILE *file) {
    if(file == NULL) return 0;  // NULLチェック
    return file->size;
}


static bool fat_set_next_cluster(uint32_t cluster, uint32_t next) {
    // 無効なクラスタ番号のチェック
    if(cluster < 2 || cluster >= 0x0FFFFFF8) return false;
    
    uint32_t fat_offset = cluster * 4;
    uint32_t sector = fat_start_lba() + (fat_offset / bpb.bytes_per_sector);
    uint32_t offset = fat_offset % bpb.bytes_per_sector;
    
    // オフセットの境界チェック
    if(offset > bpb.bytes_per_sector - 4) return false;
    
    uint8_t buf[512];
    if(!disk_read(sector, buf, 1)) return false;
    
    buf[offset]     = next & 0xFF;
    buf[offset+1]   = (next >> 8) & 0xFF;
    buf[offset+2]   = (next >> 16) & 0xFF;
    buf[offset+3]   = (buf[offset+3] & 0xF0) | ((next >> 24) & 0x0F);
    
    if(!disk_write(sector, buf, 1)) return false;
    return true;
}

// --- 初期化 ---
bool fat32_init(void) {
    uint8_t sector[512];
    if(!disk_read(0, sector, 1)) return false;
    
    bpb.bytes_per_sector    = *(uint16_t*)&sector[11];
    bpb.sectors_per_cluster = sector[13];
    bpb.reserved_sectors    = *(uint16_t*)&sector[14];
    bpb.num_fats            = sector[16];
    bpb.fat_size_sectors    = *(uint32_t*)&sector[36];
    bpb.root_cluster        = *(uint32_t*)&sector[44];
    
    // BPBパラメータの妥当性チェック
    if(bpb.bytes_per_sector == 0 || bpb.bytes_per_sector > 4096) return false;
    if(bpb.sectors_per_cluster == 0 || bpb.sectors_per_cluster > 128) return false;
    if(bpb.num_fats == 0 || bpb.num_fats > 2) return false;
    if(bpb.root_cluster < 2) return false;
    
    return true;
}

// --- ファイル検索(ルートディレクトリのみ、8.3形式) ---
bool fat32_find_file(const char *filename, FAT32_FILE *file) {
    // NULLチェック
    if(filename == NULL || file == NULL) return false;
    
    uint32_t cluster = bpb.root_cluster;
    uint8_t buf[512];
    int loop_count = 0;  // 無限ループ防止
    const int MAX_LOOPS = 10000;

    while(cluster >= 2 && cluster < 0x0FFFFFF8) {
        if(++loop_count > MAX_LOOPS) return false;  // 無限ループ防止
        
        for(uint8_t sec = 0; sec < bpb.sectors_per_cluster; sec++) {
            uint32_t lba = cluster_to_lba(cluster);
            if(lba == 0) return false;  // 無効なLBA
            
            if(!disk_read(lba + sec, buf, 1)) return false;
            
            for(int i = 0; i < 512; i += 32) {
                if(buf[i] == 0x00) return false; // ファイルなし
                if(buf[i] == 0xE5) continue;     // 削除済みエントリ
                if((buf[i+11] & 0x0F) == 0x0F) continue; // ロングファイル名無視
                
                char name[12];
                memcpy(name, &buf[i], 11);
                name[11] = '\0';
                
                // 空白削除(安全に)
                for(int j = 0; j < 11; j++) {
                    if(name[j] == ' ') name[j] = '\0';
                }
                
                if(strncmp(filename, name, 11) == 0) {
                    uint16_t high = *(uint16_t*)&buf[i+20];
                    uint16_t low = *(uint16_t*)&buf[i+26];
                    file->first_cluster = ((uint32_t)high << 16) | low;
                    file->size = *(uint32_t*)&buf[i+28];
                    memcpy(file->name, &buf[i], 11);
                    file->name[11] = '\0';  // NULL終端を保証
                    return true;
                }
            }
        }
        
        uint32_t next = fat_get_next_cluster(cluster);
        if(next >= 0x0FFFFFF8 || next < 2) break; // 終端または不正クラスタ
        cluster = next;
    }
    return false;
}

bool fat32_read_file(FAT32_FILE *file, uint8_t *buffer) {
    // NULLチェック
    if(file == NULL || buffer == NULL) return false;
    if(file->size == 0) return true;  // サイズ0のファイルは成功
    
    uint32_t cluster = file->first_cluster;
    uint32_t bytes_left = file->size;
    uint32_t bytes_per_cluster = bpb.bytes_per_sector * bpb.sectors_per_cluster;
    uint8_t buf[512];
    int loop_count = 0;  // 無限ループ防止
    const int MAX_LOOPS = 10000;

    while(cluster >= 2 && cluster < 0x0FFFFFF8 && bytes_left > 0) {
        if(++loop_count > MAX_LOOPS) return false;  // 無限ループ防止
        
        for(uint8_t sec = 0; sec < bpb.sectors_per_cluster && bytes_left > 0; sec++) {
            uint32_t lba = cluster_to_lba(cluster);
            if(lba == 0) return false;  // 無効なLBA
            
            if(!disk_read(lba + sec, buf, 1)) return false;
            
            uint32_t to_copy = (bytes_left > bpb.bytes_per_sector) ? 
                               bpb.bytes_per_sector : bytes_left;
            memcpy(buffer, buf, to_copy);
            buffer += to_copy;
            bytes_left -= to_copy;
        }
        
        uint32_t next = fat_get_next_cluster(cluster);
        if(next >= 0x0FFFFFF8 || next < 2) break;
        cluster = next;
    }
    return bytes_left == 0;
}


// --- ファイル書き込み(既存クラスタ上書き) ---
bool fat32_write_file(FAT32_FILE *file, const uint8_t *buffer) {
    // NULLチェック
    if(file == NULL || buffer == NULL) return false;
    if(file->size == 0) return true;  // サイズ0のファイルは成功
    
    uint32_t cluster = file->first_cluster;
    uint32_t bytes_left = file->size;
    uint32_t bytes_per_cluster = bpb.bytes_per_sector * bpb.sectors_per_cluster;
    int loop_count = 0;  // 無限ループ防止
    const int MAX_LOOPS = 10000;

    while(cluster >= 2 && cluster < 0x0FFFFFF8 && bytes_left > 0) {
        if(++loop_count > MAX_LOOPS) return false;  // 無限ループ防止
        
        uint32_t to_write = (bytes_left > bytes_per_cluster) ? 
                            bytes_per_cluster : bytes_left;
        
        for(uint8_t sec = 0; sec < bpb.sectors_per_cluster; sec++) {
            if(bytes_left == 0) return true;  // 書き込み完了
            
            uint32_t lba = cluster_to_lba(cluster);
            if(lba == 0) return false;  // 無効なLBA
            
            uint32_t to_write_sector = (bytes_left > bpb.bytes_per_sector) ? 
                                       bpb.bytes_per_sector : bytes_left;
            
            // セクタ全体を書き込む必要がある場合は直接書き込み
            // 部分書き込みの場合は読み込んでから書き込み
            if(to_write_sector == bpb.bytes_per_sector) {
                if(!disk_write(lba + sec, buffer, 1)) return false;
            } else {
                uint8_t temp_buf[512];
                if(!disk_read(lba + sec, temp_buf, 1)) return false;
                memcpy(temp_buf, buffer, to_write_sector);
                if(!disk_write(lba + sec, temp_buf, 1)) return false;
            }
            
            buffer += to_write_sector;
            bytes_left -= to_write_sector;
        }
        
        uint32_t next = fat_get_next_cluster(cluster);
        if(next >= 0x0FFFFFF8 || next < 2) break;
        cluster = next;
    }
    return bytes_left == 0;
}