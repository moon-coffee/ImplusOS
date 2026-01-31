// Kernel/FrameBuffer/FrameBuffer_Cursor.c
#include "FrameBuffer_Main.h"

static int cursor_w = 8;
static int cursor_h = 8;
static uint32_t cursor_backup[8*8]; // バックアップ用

void fb_draw_cursor(int x, int y, uint32_t color) {
    // 簡易：バックアップ
    for (int j = 0; j < cursor_h; j++)
        for (int i = 0; i < cursor_w; i++)
            cursor_backup[j*cursor_w + i] = fb_get_pixel(x+i, y+j);

    // 白い四角で表示
    for (int j = 0; j < cursor_h; j++)
        for (int i = 0; i < cursor_w; i++)
            fb_put_pixel(x+i, y+j, color);
}

void fb_erase_cursor(int x, int y) {
    // バックアップから復元
    for (int j = 0; j < cursor_h; j++)
        for (int i = 0; i < cursor_w; i++)
            fb_put_pixel(x+i, y+j, cursor_backup[j*cursor_w + i]);
}
