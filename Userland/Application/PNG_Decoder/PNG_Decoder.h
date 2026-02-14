#ifndef PNG_H
#define PNG_H

#include <stdint.h>

typedef struct {
    uint32_t width;
    uint32_t height;
} PNGImage;

uint32_t* png_decode_buffer(
    const uint8_t* buffer,
    uint64_t size,
    uint32_t* out_w,
    uint32_t* out_h);

#endif