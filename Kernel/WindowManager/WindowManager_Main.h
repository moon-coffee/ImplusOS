#ifndef WINDOWMANAGER_MAIN_H
#define WINDOWMANAGER_MAIN_H

#include <stdint.h>

#define MAX_WINDOWS 16

typedef struct {
    int x, y;
    int width, height;
    uint32_t bg_color;
    int visible;
    int dragging;
    int drag_offset_x, drag_offset_y;
    char title[64];  // タイトル文字列
} WINDOW;

// 初期化
void window_init(void);

// ウィンドウ作成
WINDOW* window_create(int x, int y, int w, int h, uint32_t color);
WINDOW* window_create_with_title(int x, int y, int w, int h, uint32_t color, const char *title);

// タイトル設定
void window_set_title(WINDOW *win, const char *title);

// 描画
void window_draw(WINDOW *win);
void window_draw_all(void);
void window_clear(WINDOW *win, uint32_t color);

// テキスト描画（TTFフォント）
void window_draw_text(WINDOW *win, int x, int y, const char *str, uint32_t color);
void window_draw_text_multiline(WINDOW *win, int x, int y, const char *str, uint32_t color, int line_height);
void window_draw_text_bitmap(WINDOW *win, int x, int y, const char *str, uint32_t color);

// 画像描画
void window_draw_image(WINDOW *win, int x, int y, uint8_t *rgba, int width, int height);

// マウスイベント
void window_on_mouse_move(int mx, int my, uint8_t left);

// ウィンドウ管理
void window_bring_to_front(WINDOW *win);
void window_close(WINDOW *win);
void window_show(WINDOW *win);
void window_hide(WINDOW *win);

#endif