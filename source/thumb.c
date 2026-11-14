#include "thumb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

// libjpeg por padrao chama exit() em arquivo corrompido; no 3DS isso derrubaria o app.
typedef struct {
    struct jpeg_error_mgr pub;
    jmp_buf jump;
} ErrMgr;

static void on_error(j_common_ptr cinfo)
{
    ErrMgr *e = (ErrMgr *)cinfo->err;
    longjmp(e->jump, 1);
}

static void on_message(j_common_ptr cinfo, int level)
{
    (void)cinfo; (void)level;   // warnings de JPEG corrompido: silencio
}

static int denom_for(int img_w, int img_h, int w, int h)
{
    // maior divisor de escala que ainda cobre w x h
    static const int dens[] = {8, 4, 2, 1};
    for (unsigned i = 0; i < sizeof dens / sizeof dens[0]; i++) {
        int d = dens[i];
        if ((img_w + d - 1) / d >= w && (img_h + d - 1) / d >= h) return d;
    }
    return 1;
}

bool thumb_decode_rgb(const char *path, int w, int h, uint8_t *rgb)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    struct jpeg_decompress_struct cinfo;
    ErrMgr err;
    uint8_t *line = NULL;
    bool ok = false, created = false;

    memset(&cinfo, 0, sizeof cinfo);
    memset(rgb, 0, (size_t)w * h * 3);
    cinfo.err = jpeg_std_error(&err.pub);
    err.pub.error_exit = on_error;
    err.pub.emit_message = on_message;
    if (setjmp(err.jump)) goto done;

    jpeg_create_decompress(&cinfo);
    created = true;
    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, TRUE);

    int d = denom_for(cinfo.image_width, cinfo.image_height, w, h);
    cinfo.scale_num = 1;
    cinfo.scale_denom = d;
    cinfo.out_color_space = JCS_RGB;
    cinfo.dct_method = JDCT_IFAST;

    jpeg_start_decompress(&cinfo);

    int sw = (int)cinfo.output_width;
    int sh = (int)cinfo.output_height;
    if (!sw || !sh) goto done;
    if (sw < w || sh < h) { w = sw; h = sh; }      // imagem pequena: nao inventa pixel

    line = malloc((size_t)sw * 3);
    if (!line) goto done;

    for (int y = 0; y < sh; y++) {
        JSAMPROW row = line;
        jpeg_read_scanlines(&cinfo, &row, 1);

        // linhas de saida que caem nesta linha de entrada (vizinho mais proximo)
        int oy0 = y * h / sh;
        int oy1 = (y + 1) * h / sh;
        if (oy1 <= oy0) continue;
        for (int oy = oy0; oy < oy1 && oy < h; oy++) {
            uint8_t *dst = rgb + (size_t)oy * w * 3;
            for (int ox = 0; ox < w; ox++) {
                const uint8_t *s = line + (size_t)(ox * sw / w) * 3;
                dst[ox * 3 + 0] = s[0];
                dst[ox * 3 + 1] = s[1];
                dst[ox * 3 + 2] = s[2];
            }
        }
    }
    ok = true;

done:
    free(line);
    if (created) jpeg_destroy_decompress(&cinfo);
    fclose(f);
    return ok;
}

//---------------------------------------------------------------------------------
void fb_px(uint8_t *fb, int bpp, int x, int y, int scr_w, int scr_h,
           uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || x >= scr_w || y < 0 || y >= scr_h) return;
    uint8_t *p = fb + (size_t)(x * FB_ROW_PX + (FB_ROW_PX - 1 - y)) * bpp;
    switch (bpp) {
        case 3:                                   // BGR8
            p[0] = b; p[1] = g; p[2] = r;
            break;
        case 4:                                   // BGR8A
            p[0] = b; p[1] = g; p[2] = r; p[3] = 0xFF;
            break;
        default: {                                // 565 / 5A1 / RGBA4
            uint16_t c = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
            memcpy(p, &c, sizeof c);
            break;
        }
    }
}

void thumb_blit(const uint8_t *rgb, int w, int h,
                uint8_t *fb, int bpp, int dx, int dy, int scr_w, int scr_h)
{
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const uint8_t *s = rgb + ((size_t)y * w + x) * 3;
            fb_px(fb, bpp, dx + x, dy + y, scr_w, scr_h, s[0], s[1], s[2]);
        }
    }
}

void thumb_fill(uint8_t *fb, int bpp, int dx, int dy, int w, int h,
                int scr_w, int scr_h, uint8_t r, uint8_t g, uint8_t b)
{
    for (int y = dy; y < dy + h; y++)
        for (int x = dx; x < dx + w; x++)
            fb_px(fb, bpp, x, y, scr_w, scr_h, r, g, b);
}

void thumb_border(uint8_t *fb, int bpp, int dx, int dy, int w, int h, int t,
                  int scr_w, int scr_h, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < t; i++) {
        for (int x = dx - i; x < dx + w + i; x++) {
            fb_px(fb, bpp, x, dy - i, scr_w, scr_h, r, g, b);
            fb_px(fb, bpp, x, dy + h - 1 + i, scr_w, scr_h, r, g, b);
        }
        for (int y = dy - i; y < dy + h + i; y++) {
            fb_px(fb, bpp, dx - i, y, scr_w, scr_h, r, g, b);
            fb_px(fb, bpp, dx + w - 1 + i, y, scr_w, scr_h, r, g, b);
        }
    }
}
