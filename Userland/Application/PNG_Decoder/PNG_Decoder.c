#include <stdint.h>
#include "../../Syscalls.h"
#include "PNG_Decoder.h"

extern void* kmalloc(uint32_t size);
extern void kfree(void* ptr);

static uint32_t read_be32(const uint8_t* p) {
    return (p[0] << 24) |
           (p[1] << 16) |
           (p[2] << 8)  |
           (p[3]);
}

static void png_unfilter(uint8_t* data,
                         uint32_t width,
                         uint32_t height)
{
    uint32_t stride = width * 4;
    uint8_t* prev = NULL;
    uint8_t* cur = data;

    for (uint32_t y = 0; y < height; y++) {

        uint8_t filter = cur[0];
        uint8_t* row = cur + 1;

        switch (filter) {

        case 0:
            break;

        case 1:
            for (uint32_t x = 4; x < stride; x++)
                row[x] += row[x - 4];
            break;

        case 2:
            if (prev)
                for (uint32_t x = 0; x < stride; x++)
                    row[x] += prev[x];
            break;

        case 3:
            for (uint32_t x = 0; x < stride; x++) {
                uint8_t left = (x >= 4) ? row[x - 4] : 0;
                uint8_t up   = prev ? prev[x] : 0;
                row[x] += (left + up) / 2;
            }
            break;

        case 4:
            for (uint32_t x = 0; x < stride; x++) {
                uint8_t a = (x >= 4) ? row[x - 4] : 0;
                uint8_t b = prev ? prev[x] : 0;
                uint8_t c = (x >= 4 && prev) ? prev[x - 4] : 0;

                int p  = a + b - c;
                int pa = (p > a) ? (p - a) : (a - p);
                int pb = (p > b) ? (p - b) : (b - p);
                int pc = (p > c) ? (p - c) : (c - p);

                uint8_t pr =
                    (pa <= pb && pa <= pc) ? a :
                    (pb <= pc) ? b : c;

                row[x] += pr;
            }
            break;

        default:
            break;
        }

        prev = row;
        cur += stride + 1;
    }
}

static uint8_t* zlib_decompress_uncompressed(
    const uint8_t* data,
    uint32_t size,
    uint32_t* out_size)
{
    if (size < 6)
        return NULL;

    const uint8_t* p = data + 2;

    uint8_t btype = (p[0] >> 1) & 0x3;
    if (btype != 0)
        return NULL;

    p++;

    uint16_t len  = p[0] | (p[1] << 8);
    p += 4;

    uint8_t* out = kmalloc(len);
    if (!out)
        return NULL;

    memcpy(out, p, len);
    *out_size = len;

    return out;
}

uint32_t* png_decode_buffer(
    const uint8_t* buffer,
    uint64_t size,
    uint32_t* out_w,
    uint32_t* out_h)
{
    static const uint8_t sig[8] =
        {137,80,78,71,13,10,26,10};

    if (size < 8 || memcmp(buffer, sig, 8) != 0)
        return NULL;

    const uint8_t* p = buffer + 8;

    uint32_t width = 0;
    uint32_t height = 0;

    const uint8_t* idat = NULL;
    uint32_t idat_size = 0;

    while (p < buffer + size) {

        uint32_t len  = read_be32(p); p += 4;
        uint32_t type = read_be32(p); p += 4;

        if (type == 0x49484452) {

            width  = read_be32(p);
            height = read_be32(p + 4);

            uint8_t bit_depth = p[8];
            uint8_t color     = p[9];
            uint8_t interlace = p[12];

            if (bit_depth != 8 ||
                color != 6 ||
                interlace != 0)
                return NULL;

        }
        else if (type == 0x49444154) {
            idat = p;
            idat_size = len;
        }
        else if (type == 0x49454E44) {
            break;
        }

        p += len + 4;
    }

    if (!idat)
        return NULL;

    uint32_t decomp_size;
    uint8_t* decomp =
        zlib_decompress_uncompressed(
            idat, idat_size, &decomp_size);

    if (!decomp)
        return NULL;

    png_unfilter(decomp, width, height);

    uint32_t* out =
        kmalloc(width * height * 4);
    if (!out) {
        kfree(decomp);
        return NULL;
    }

    uint8_t* src = decomp;
    uint32_t stride = width * 4;

    for (uint32_t y = 0; y < height; y++) {
        memcpy(
            (uint8_t*)&out[y * width],
            src + 1,
            stride);
        src += stride + 1;
    }

    kfree(decomp);

    *out_w = width;
    *out_h = height;

    return out;
}