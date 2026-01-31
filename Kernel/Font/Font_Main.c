#include "Font_Main.h"
#include "../FrameBuffer/FrameBuffer_Main.h"
#include "../Drivers/FileSystem/FAT32/FAT32_Main.h"
#include "../Memory/Memory_Main.h"
#include <stdint.h>

// stb_trueypeのカスタムアロケータを定義（インクルード前に必須！）
#define STBTT_malloc(x,u)  kmalloc((uint32_t)(x))
#define STBTT_free(x,u)    kfree(x)

// stb_truetypeの実装を有効化（このファイルでのみ）
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// 描画関数（固定フォント用）
void draw_char(const uint8_t *glyph, uint32_t x, uint32_t y, uint32_t color) {
    for(int row=0; row<16; row++){
        uint8_t bits = glyph[row];
        for(int col=0; col<8; col++){
            if(bits & (1<<(7-col))){
                fb_put_pixel(x+col, y+row, color);
            }
        }
    }
}

// TTF フォントキャッシュ
static unsigned char *cached_ttf_buffer = NULL;
static stbtt_fontinfo cached_font = {0};

// TTF フォント初期化
int init_ttf_font(void) {
    FAT32_FILE file = {0};
    if(!fat32_find_file("NOTOSANSJP   TTF", &file)) {
        return 0;
    }

    // FAT32_Main.cで定義されている関数を使用
    uint32_t filesize = fat32_get_file_size(&file);
    if(filesize == 0) return 0;

    cached_ttf_buffer = (unsigned char*)kmalloc(filesize);
    if(!cached_ttf_buffer) return 0;

    if(!fat32_read_file(&file, cached_ttf_buffer)) {
        kfree(cached_ttf_buffer);
        cached_ttf_buffer = NULL;
        return 0;
    }

    if(!stbtt_InitFont(&cached_font, cached_ttf_buffer, 0)) {
        kfree(cached_ttf_buffer);
        cached_ttf_buffer = NULL;
        return 0;
    }

    return 1;
}

// TTF フォントから文字描画
void draw_char_from_ttf(int x, int y, char c, uint32_t color) {
    if(!cached_ttf_buffer) {
        if(!init_ttf_font()) return;
    }

    float scale = stbtt_ScaleForPixelHeight(&cached_font, 16);

    int w, h, xoff, yoff;
    unsigned char *bitmap = stbtt_GetCodepointBitmap(&cached_font, 
                                                      scale, scale, 
                                                      (int)c, 
                                                      &w, &h, 
                                                      &xoff, &yoff);
    if(!bitmap) return;

    for(int row=0; row<h; row++){
        for(int col=0; col<w; col++){
            uint8_t alpha = bitmap[row*w + col];
            if(alpha > 128){
                fb_put_pixel(x + col + xoff, y + row + yoff, color);
            }
        }
    }

    stbtt_FreeBitmap(bitmap, NULL);
}

// 文字列描画関数
void draw_string_from_ttf(int x, int y, const char *str, uint32_t color) {
    if(!cached_ttf_buffer) {
        if(!init_ttf_font()) return;
    }

    float scale = stbtt_ScaleForPixelHeight(&cached_font, 16);
    int current_x = x;

    while(*str) {
        int advance, lsb;
        int glyph = stbtt_FindGlyphIndex(&cached_font, (int)*str);
        
        draw_char_from_ttf(current_x, y, *str, color);
        
        stbtt_GetGlyphHMetrics(&cached_font, glyph, &advance, &lsb);
        current_x += (int)(advance * scale);
        
        if(*(str+1)) {
            int next_glyph = stbtt_FindGlyphIndex(&cached_font, (int)*(str+1));
            current_x += (int)(stbtt_GetGlyphKernAdvance(&cached_font, 
                                                          glyph, 
                                                          next_glyph) * scale);
        }
        
        str++;
    }
}

// クリーンアップ
void cleanup_ttf_font(void) {
    if(cached_ttf_buffer) {
        kfree(cached_ttf_buffer);
        cached_ttf_buffer = NULL;
    }
}