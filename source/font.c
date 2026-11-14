#include "font.h"
#include <string.h>
#include "thumb.h"

// definido em libctru (default_font.bin.o): 256 glifos de 8 bytes, 1 bit por pixel,
// MSB = pixel mais a esquerda.
extern const unsigned char default_font_bin[];

#define GLYPH_W 8
#define GLYPH_H 8

void font_draw(uint8_t *fb, int bpp, int x, int y, const char *s,
               int scr_w, int scr_h, uint8_t r, uint8_t g, uint8_t b)
{
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        const unsigned char *g_ = default_font_bin + (size_t)(*p) * GLYPH_H;
        for (int row = 0; row < GLYPH_H; row++) {
            unsigned char bits = g_[row];
            if (!bits) continue;
            for (int col = 0; col < GLYPH_W; col++) {
                if (bits & (0x80 >> col))
                    fb_px(fb, bpp, x + col, y + row, scr_w, scr_h, r, g, b);
            }
        }
        x += GLYPH_W;
    }
}

int font_width(const char *s)
{
    return (int)strlen(s) * GLYPH_W;
}
