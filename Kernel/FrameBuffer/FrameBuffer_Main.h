#ifndef FRAMEBUFFER_MAIN_H
#define FRAMEBUFFER_MAIN_H

#include <stdint.h>

extern uint32_t screen_w;
extern uint32_t screen_h;

void fb_init(void *base, uint32_t w, uint32_t h, uint32_t pitch);
uint32_t fb_get_pixel(uint32_t x, uint32_t y);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_clear(uint32_t color);
void fb_draw_cursor(int x, int y, uint32_t color);
void fb_set_cursor(int x, int y);

#endif
