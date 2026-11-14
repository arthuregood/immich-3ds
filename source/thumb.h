#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Decodifica o JPEG (ou o primeiro frame de um MPO) reduzindo para w x h.
// Preenche rgb com w*h*3 bytes (R,G,B). Decodifica em escala reduzida (1/8, 1/4...)
// para nao gastar RAM nem CPU com 640x480 quando so precisamos de 106x80.
bool thumb_decode_rgb(const char *path, int w, int h, uint8_t *rgb);

// --- Desenho no framebuffer do 3DS ---
// O framebuffer NAO e linear: cada linha da memoria tem 240 pixels e o indice e
//   x*240 + (239 - y)
// (mesma conta que o console.c do libctru usa). bpp = 2 (565) ou 3 (BGR8).
// As coordenadas sao as da tela: 320x240 embaixo, 400x240 em cima.
#define FB_ROW_PX 240

void fb_px(uint8_t *fb, int bpp, int x, int y, int scr_w, int scr_h,
           uint8_t r, uint8_t g, uint8_t b);

void thumb_blit(const uint8_t *rgb, int w, int h,
                uint8_t *fb, int bpp, int dx, int dy, int scr_w, int scr_h);

void thumb_fill(uint8_t *fb, int bpp, int dx, int dy, int w, int h,
                int scr_w, int scr_h, uint8_t r, uint8_t g, uint8_t b);

void thumb_border(uint8_t *fb, int bpp, int dx, int dy, int w, int h, int t,
                  int scr_w, int scr_h, uint8_t r, uint8_t g, uint8_t b);
