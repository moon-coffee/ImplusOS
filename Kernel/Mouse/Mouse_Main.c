#include "Mouse_Main.h"
#include "../FrameBuffer/FrameBuffer_Main.h"
#include "../WindowManager/WindowManager_Main.h"
#include <stdint.h>

#define MOUSE_DATA_PORT   0x60
#define MOUSE_STATUS_PORT 0x64
#define MOUSE_CMD_PORT    0x64

static MOUSE_STATE mouse;
static uint8_t packet[3];
static uint8_t packet_index = 0;

static void wait_output(void){
    while ((inb(MOUSE_STATUS_PORT) & 1) == 0);
}

static void wait_input(void){
    while ((inb(MOUSE_STATUS_PORT) & 2) != 0);
}

static void write_mouse(uint8_t cmd){
    wait_input();
    outb(MOUSE_CMD_PORT, 0xD4);
    wait_input();
    outb(MOUSE_DATA_PORT, cmd);
}

static void mouse_handler(void){
    uint8_t data = inb(MOUSE_DATA_PORT);
    packet[packet_index++] = data;
    if (packet_index == 3){
        int8_t dx = packet[1];
        int8_t dy = packet[2];
        mouse.x += dx;
        mouse.y -= dy; // y座標は反転
        if (mouse.x < 0) mouse.x = 0;
        if (mouse.y < 0) mouse.y = 0;

        mouse.buttons = packet[0] & 0x07;

        // ウィンドウドラッグ
        window_on_mouse_move(mouse.x, mouse.y, mouse.buttons);

        packet_index = 0;
    }
}

void mouse_init(void){
    mouse.x = 100;
    mouse.y = 100;
    mouse.buttons = 0;

    // マウス有効化
    write_mouse(0xF4);
    // 割り込みハンドラ登録（IRQ12）
    register_interrupt_handler(44, mouse_handler); // 32 + 12 = 44
}

MOUSE_STATE mouse_get_state(void){
    return mouse;
}

void mouse_update(void){
    // IRQベースなので特になし
}
