#include "form.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void form_decode(const char *src, size_t len, char *out, size_t outsz)
{
    size_t o = 0;
    for (size_t i = 0; i < len && o + 1 < outsz; i++) {
        if (src[i] == '+') {
            out[o++] = ' ';
        } else if (src[i] == '%' && i + 2 < len) {
            int h = hexval(src[i + 1]), l = hexval(src[i + 2]);
            if (h >= 0 && l >= 0) {
                out[o++] = (char)((h << 4) | l);
                i += 2;
            } else {
                out[o++] = src[i];
            }
        } else {
            out[o++] = src[i];
        }
    }
    out[o] = 0;
}

bool form_get(const char *body, size_t len, const char *key, char *out, size_t outsz)
{
    size_t klen = strlen(key);
    size_t i = 0;

    while (i < len) {
        size_t start = i;
        while (i < len && body[i] != '&') i++;
        size_t end = i;                       // [start, end) = par
        if (i < len) i++;                     // pula '&'

        const char *eq = memchr(body + start, '=', end - start);
        if (!eq) continue;
        size_t kend = (size_t)(eq - body);
        if (kend - start != klen) continue;
        if (strncasecmp(body + start, key, klen)) continue;

        char tmp[512];
        form_decode(body + kend + 1, end - kend - 1, tmp, sizeof tmp);

        // tira espacos e barra final do servidor
        char *v = tmp;
        while (*v == ' ') v++;
        size_t vl = strlen(v);
        while (vl && (v[vl - 1] == ' ' || v[vl - 1] == '\r' || v[vl - 1] == '\n')) v[--vl] = 0;
        snprintf(out, outsz, "%s", v);
        return true;
    }
    return false;
}

long form_content_length(const char *headers, size_t len)
{
    const char *p = headers;
    while (p && (size_t)(p - headers) < len) {
        if (!strncasecmp(p, "content-length:", 15)) {
            const char *v = p + 15;
            while (*v == ' ' || *v == '\t') v++;
            return strtol(v, NULL, 10);
        }
        const char *nl = memchr(p, '\n', len - (size_t)(p - headers));
        if (!nl) break;
        p = nl + 1;
    }
    return -1;
}
