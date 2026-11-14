#pragma once
#include <stdint.h>

// Fonte 8x8 do proprio libctru (simbolo default_font_bin, 256 chars x 8 bytes).
// Sem console, sem romfs, sem dependencia nova.
void font_draw(uint8_t *fb, int bpp, int x, int y, const char *s,
               int scr_w, int scr_h, uint8_t r, uint8_t g, uint8_t b);

int font_width(const char *s);
