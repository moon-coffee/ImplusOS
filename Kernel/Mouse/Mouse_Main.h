#pragma once
#include <stdint.h>

typedef struct {
    int32_t x, y;
    uint8_t buttons;
} MOUSE_STATE;

void mouse_init(void);
MOUSE_STATE mouse_get_state(void);
void mouse_update(void);
