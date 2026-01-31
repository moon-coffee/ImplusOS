#include "WindowManager_Main.h"
#include "../FrameBuffer/FrameBuffer_Main.h"
#include "../Font/Font_Main.h"
#include <stdint.h>

static WINDOW windows[MAX_WINDOWS];
static int window_count = 0;
static WINDOW *dragging_win = 0;

// 内部：矩形塗りつぶし
static void fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            fb_put_pixel(x + i, y + j, color);
}

// 内部：矩形の枠線描画
static void draw_rect_border(int x, int y, int w, int h, uint32_t color, int thickness) {
    // 上辺
    fill_rect(x, y, w, thickness, color);
    // 下辺
    fill_rect(x, y + h - thickness, w, thickness, color);
    // 左辺
    fill_rect(x, y, thickness, h, color);
    // 右辺
    fill_rect(x + w - thickness, y, thickness, h, color);
}

// 初期化
void window_init(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].visible = 0;
        windows[i].dragging = 0;
        windows[i].x = 0;
        windows[i].y = 0;
        windows[i].width = 0;
        windows[i].height = 0;
        windows[i].bg_color = 0;
        windows[i].title[0] = '\0';
        windows[i].drag_offset_x = 0;
        windows[i].drag_offset_y = 0;
    }
    window_count = 0;
    dragging_win = 0;
}

// ウィンドウ作成（タイトル付き）
WINDOW* window_create(int x, int y, int w, int h, uint32_t color) {
    if (window_count >= MAX_WINDOWS) return 0;
    
    WINDOW *win = &windows[window_count++];
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->bg_color = color;
    win->visible = 1;
    win->dragging = 0;
    win->title[0] = '\0';
    win->drag_offset_x = 0;
    win->drag_offset_y = 0;
    
    return win;
}

// タイトル付きウィンドウ作成
WINDOW* window_create_with_title(int x, int y, int w, int h, uint32_t color, const char *title) {
    WINDOW *win = window_create(x, y, w, h, color);
    if (win && title) {
        window_set_title(win, title);
    }
    return win;
}

// ウィンドウにタイトルを設定
void window_set_title(WINDOW *win, const char *title) {
    if (!win || !title) return;
    
    int i = 0;
    while (title[i] && i < 63) {
        win->title[i] = title[i];
        i++;
    }
    win->title[i] = '\0';
}

// 単体ウィンドウ描画（タイトルバー付き）
void window_draw(WINDOW *win) {
    if (!win || !win->visible) return;
    
    const int TITLEBAR_HEIGHT = 24;
    const int BORDER_WIDTH = 2;
    
    // タイトルバーの色（背景色を少し暗くする）
    uint32_t titlebar_color = win->bg_color & 0xC0C0C0;
    
    // タイトルバー描画
    fill_rect(win->x, win->y, win->width, TITLEBAR_HEIGHT, titlebar_color);
    
    // ウィンドウ本体描画
    fill_rect(win->x, win->y + TITLEBAR_HEIGHT, win->width, 
              win->height - TITLEBAR_HEIGHT, win->bg_color);
    
    // 枠線描画
    draw_rect_border(win->x, win->y, win->width, win->height, 0x808080, BORDER_WIDTH);
    
    // タイトル描画（TTFフォント使用）
    if (win->title[0] != '\0') {
        draw_string_from_ttf(win->x + 8, win->y + 4, win->title, 0xFFFFFF);
    }
    
    // 閉じるボタン（×印）の描画
    int close_btn_x = win->x + win->width - 20;
    int close_btn_y = win->y + 4;
    draw_string_from_ttf(close_btn_x, close_btn_y, "X", 0xFF0000);
}

// すべてのウィンドウ描画
void window_draw_all(void) {
    for (int i = 0; i < window_count; i++) {
        window_draw(&windows[i]);
    }
}

// 文字描画（TTFフォント使用）
void window_draw_text(WINDOW *win, int x, int y, const char *str, uint32_t color) {
    if (!win || !str) return;
    
    const int TITLEBAR_HEIGHT = 24;
    
    // ウィンドウの絶対座標に変換（タイトルバーの下から）
    int abs_x = win->x + x;
    int abs_y = win->y + TITLEBAR_HEIGHT + y;
    
    // TTFフォントで文字列描画
    draw_string_from_ttf(abs_x, abs_y, str, color);
}
// 複数行テキスト描画（TTFフォント）
void window_draw_text_multiline(WINDOW *win, int x, int y, const char *str, uint32_t color, int line_height) {
    if (!win || !str) return;
    
    const int TITLEBAR_HEIGHT = 24;
    int current_y = y;
    int current_x = x;
    
    while (*str) {
        if (*str == '\n') {
            current_y += line_height;
            current_x = x;
            str++;
            continue;
        }
        
        // 1文字ずつ描画
        char temp[2] = {*str, '\0'};
        draw_string_from_ttf(win->x + current_x, win->y + TITLEBAR_HEIGHT + current_y, temp, color);
        
        // 次の文字位置を計算（簡易的に固定幅を使用）
        current_x += 12; // TTFフォントの平均幅を仮定
        str++;
    }
}

// ウィンドウにバッファをコピーする
void window_draw_image(WINDOW *win, int x, int y, uint8_t *rgba, int width, int height) {
    if (!win || !rgba) return;
    
    const int TITLEBAR_HEIGHT = 24;
    
    for(int j = 0; j < height; j++){
        for(int i = 0; i < width; i++){
            int img_idx = (j*width + i) * 4;
            uint8_t r = rgba[img_idx + 0];
            uint8_t g = rgba[img_idx + 1];
            uint8_t b = rgba[img_idx + 2];
            uint8_t a = rgba[img_idx + 3];
            
            // アルファ値が0の場合はスキップ（透過）
            if (a < 128) continue;
            
            uint32_t color = (r << 16) | (g << 8) | b;
            fb_put_pixel(win->x + x + i, win->y + TITLEBAR_HEIGHT + y + j, color);
        }
    }
}

// ウィンドウクリア
void window_clear(WINDOW *win, uint32_t color) {
    if (!win) return;
    
    const int TITLEBAR_HEIGHT = 24;
    fill_rect(win->x, win->y + TITLEBAR_HEIGHT, win->width, 
              win->height - TITLEBAR_HEIGHT, color);
}

// マウス移動・ドラッグ
void window_on_mouse_move(int mx, int my, uint8_t left) {
    const int TITLEBAR_HEIGHT = 24;
    
    // カーソル更新
    fb_erase_cursor(mx, my);
    fb_draw_cursor(mx, my, 0xFFFFFFFF);

    if (dragging_win) {
        if (left) {
            // ドラッグ中
            dragging_win->x = mx - dragging_win->drag_offset_x;
            dragging_win->y = my - dragging_win->drag_offset_y;
            
            // 画面外に出ないように制限（オプション）
            if (dragging_win->x < 0) dragging_win->x = 0;
            if (dragging_win->y < 0) dragging_win->y = 0;
        } else {
            // ドラッグ終了
            dragging_win->dragging = 0;
            dragging_win = 0;
        }
    } else if (left) {
        // 新しいドラッグ開始を検出
        // 後ろから（最前面）のウィンドウから判定
        for (int i = window_count - 1; i >= 0; i--) {
            WINDOW *w = &windows[i];
            if (!w->visible) continue;
            
            // タイトルバー領域でクリックされたか判定
            if (mx >= w->x && mx < w->x + w->width &&
                my >= w->y && my < w->y + TITLEBAR_HEIGHT) {
                
                // 閉じるボタンの判定
                int close_btn_x = w->x + w->width - 20;
                if (mx >= close_btn_x && mx < close_btn_x + 16) {
                    // 閉じるボタンがクリックされた
                    w->visible = 0;
                    return;
                }
                
                // ドラッグ開始
                dragging_win = w;
                w->dragging = 1;
                w->drag_offset_x = mx - w->x;
                w->drag_offset_y = my - w->y;
                
                // ウィンドウを最前面に移動（オプション）
                window_bring_to_front(w);
                break;
            }
        }
    }
}

// ウィンドウを最前面に移動
void window_bring_to_front(WINDOW *win) {
    if (!win) return;
    
    // winが最後でなければ、最後に移動
    int win_index = -1;
    for (int i = 0; i < window_count; i++) {
        if (&windows[i] == win) {
            win_index = i;
            break;
        }
    }
    
    if (win_index == -1 || win_index == window_count - 1) return;
    
    // 一時保存
    WINDOW temp = windows[win_index];
    
    // 後ろにシフト
    for (int i = win_index; i < window_count - 1; i++) {
        windows[i] = windows[i + 1];
    }
    
    // 最後に配置
    windows[window_count - 1] = temp;
}

// ウィンドウを閉じる
void window_close(WINDOW *win) {
    if (!win) return;
    win->visible = 0;
}

// ウィンドウを表示
void window_show(WINDOW *win) {
    if (!win) return;
    win->visible = 1;
}

// ウィンドウを非表示
void window_hide(WINDOW *win) {
    if (!win) return;
    win->visible = 0;
}