#include "FrameBuffer_Main.h"
#include <stddef.h>

typedef struct {
    uint8_t b, g, r, a;
} __attribute__((packed)) PIXEL;

static PIXEL *fb = NULL;       // フレームバッファのベースアドレス
static uint32_t pitch = 0;     // 1行あたりのPIXEL数
static int cursor_x = 0;
static int cursor_y = 0;
static uint32_t cursor_color = 0xFFFFFFFF; // 白
static uint32_t cursor_bg    = 0x00000000; // 黒

uint32_t screen_w = 0;
uint32_t screen_h = 0;

// フレームバッファ初期化
void fb_init(void *base, uint32_t w, uint32_t h, uint32_t p) {
    fb = (PIXEL*)base;
    screen_w = w;
    screen_h = h;
    pitch = p;
}

// 指定座標のピクセルを取得
uint32_t fb_get_pixel(uint32_t x, uint32_t y) {
    if (!fb || x >= screen_w || y >= screen_h)
        return 0;

    PIXEL *p = fb + y * pitch + x;
    return (p->r << 16) | (p->g << 8) | p->b;
}

// 指定座標にピクセルを書き込む
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb || x >= screen_w || y >= screen_h) return;

    PIXEL *p = fb + y * pitch + x;
    p->r = (color >> 16) & 0xFF;
    p->g = (color >> 8) & 0xFF;
    p->b = color & 0xFF;
    p->a = 0xFF; // 透明度固定
}

// 画面全体を指定色で塗りつぶす
void fb_clear(uint32_t color) {
    for (uint32_t y = 0; y < screen_h; y++) {
        for (uint32_t x = 0; x < screen_w; x++) {
            fb_put_pixel(x, y, color);
        }
    }
}

// カーソル位置を設定
void fb_set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}
