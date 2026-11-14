#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Decodifica o JPEG (ou o primeiro frame de um MPO) reduzindo para w x h.
// Preenche rgb com w*h*3 bytes (R,G,B). Decodifica em escala reduzida (1/8, 1/4...)
// para nao gastar RAM nem CPU com 640x480 quando so precisamos de 53x40.
bool thumb_decode_rgb(const char *path, int w, int h, uint8_t *rgb);

// Copia rgb para o framebuffer BGR8 em (dx,dy), com clipping.
void thumb_blit_bgr(const uint8_t *rgb, int w, int h,
                    uint8_t *fb, int stride, int dx, int dy, int fbw, int fbh);

// Moldura de t pixels em volta do retangulo.
void thumb_border(uint8_t *fb, int stride, int dx, int dy, int w, int h, int t,
                  int fbw, int fbh, uint8_t r, uint8_t g, uint8_t b);
