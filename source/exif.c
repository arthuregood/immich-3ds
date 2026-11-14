#include "exif.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static uint32_t rd16(const uint8_t *p, int be)
{
    return be ? (uint32_t)((p[0] << 8) | p[1]) : (uint32_t)((p[1] << 8) | p[0]);
}

static uint32_t rd32(const uint8_t *p, int be)
{
    return be ? ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3]
              : ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | p[0];
}

static void parse_tiff(const uint8_t *t, int n, char *out, size_t outsz)
{
    if (n < 8) return;
    int be;
    if (t[0] == 'I' && t[1] == 'I')      be = 0;
    else if (t[0] == 'M' && t[1] == 'M') be = 1;
    else return;
    if (rd16(t + 2, be) != 42) return;

    uint32_t ifd = rd32(t + 4, be);
    if (ifd + 2 > (uint32_t)n) return;

    int cnt = (int)rd16(t + ifd, be);
    for (int i = 0; i < cnt; i++) {
        uint32_t off = ifd + 2 + i * 12;
        if (off + 12 > (uint32_t)n) return;
        uint32_t tag   = rd16(t + off, be);
        uint32_t type  = rd16(t + off + 2, be);
        uint32_t count = rd32(t + off + 4, be);
        if (tag != 0x0132 && tag != 0x9003) continue;
        if (type != 2 || count < 19) continue;          // 2 = ASCII, precisa de "YYYY:MM:DD HH:MM:SS"
        const uint8_t *src;
        if (count <= 4) {
            src = t + off + 8;                          // valor cabe no campo
        } else {
            uint32_t v = rd32(t + off + 8, be);
            if (v + 19 > (uint32_t)n) return;
            src = t + v;
        }
        snprintf(out, outsz, "%.19s", (const char *)src);
        return;
    }
}

bool exif_datetime(const char *path, char *out, size_t outsz)
{
    out[0] = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint8_t buf[4096];
    if (fread(buf, 1, 2, f) != 2 || buf[0] != 0xFF || buf[1] != 0xD8) { fclose(f); return false; }

    while (1) {
        int c1 = fgetc(f);
        if (c1 != 0xFF) break;
        int c2 = fgetc(f);
        if (c2 == EOF || c2 == 0xFF) continue;
        if (c2 == 0xD8 || c2 == 0x01 || (c2 >= 0xD0 && c2 <= 0xD7)) continue;  // marcadores sem tamanho
        int hi = fgetc(f), lo = fgetc(f);
        if (hi == EOF || lo == EOF) break;
        int len = (hi << 8) | lo;
        if (len < 2) break;
        int body = len - 2;

        if (c2 == 0xE1) {                                                       // APP1
            int n = body > (int)sizeof buf ? (int)sizeof buf : body;
            if (fread(buf, 1, n, f) != (size_t)n) break;
            if (n > 6 && memcmp(buf, "Exif\0\0", 6) == 0) parse_tiff(buf + 6, n - 6, out, outsz);
            if (out[0]) break;
            if (body > n) fseek(f, body - n, SEEK_CUR);
        } else {
            if (c2 == 0xDA) break;                                              // inicio dos dados da imagem
            fseek(f, body, SEEK_CUR);
        }
    }
    fclose(f);
    return out[0] != 0;
}

void exif_to_iso(const char *exif, const char *tz, char *out, size_t outsz)
{
    if (strlen(exif) < 19) { out[0] = 0; return; }
    snprintf(out, outsz, "%.4s-%.2s-%.2sT%.8s.000%s",
             exif, exif + 5, exif + 8, exif + 11, tz && tz[0] ? tz : "Z");
}

void exif_to_short(const char *exif, char *out, size_t outsz)
{
    if (strlen(exif) < 16) { snprintf(out, outsz, "--"); return; }
    snprintf(out, outsz, "%.4s-%.2s-%.2s %.5s", exif, exif + 5, exif + 8, exif + 11);
}
