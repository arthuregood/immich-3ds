// Teste do parser EXIF e da formatacao de data. Roda no host, sem 3DS.
//   ./tests/run.sh
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "exif.h"

static int failures;

#define CHECK(cond) do { \
    if (!(cond)) { printf("  FALHOU: %s (linha %d)\n", #cond, __LINE__); failures++; } \
} while (0)

#define CHECK_STR(got, want) do { \
    if (strcmp((got), (want))) { \
        printf("  FALHOU: esperado \"%s\", veio \"%s\" (linha %d)\n", (want), (got), __LINE__); \
        failures++; \
    } \
} while (0)

int main(void)
{
    char dt[20], iso[40], shrt[24], out[40];

    // JPEG com EXIF little-endian
    memset(dt, 0, sizeof dt);
    CHECK(exif_datetime("tests/data/exif_le.jpg", dt, sizeof dt));
    CHECK_STR(dt, "2026:11:13 22:15:23");
    exif_to_iso(dt, "-03:00", iso, sizeof iso);
    CHECK_STR(iso, "2026-11-13T22:15:23.000-03:00");
    exif_to_short(dt, shrt, sizeof shrt);
    CHECK_STR(shrt, "2026-11-13 22:15");

    // mesma coisa, big-endian
    memset(dt, 0, sizeof dt);
    CHECK(exif_datetime("tests/data/exif_be.jpg", dt, sizeof dt));
    CHECK_STR(dt, "2019:07:04 09:05:06");
    exif_to_iso(dt, "+02:00", iso, sizeof iso);
    CHECK_STR(iso, "2019-07-04T09:05:06.000+02:00");

    // sem EXIF: nao quebra e devolve false
    memset(out, 'x', sizeof out);
    CHECK(!exif_datetime("tests/data/noexif.jpg", out, sizeof out));

    // nao e JPEG
    CHECK(!exif_datetime("tests/data/notjpeg.bin", out, sizeof out));

    // arquivo inexistente
    CHECK(!exif_datetime("tests/data/nao-existe.jpg", out, sizeof out));

    // entrada invalida nao gera string gigante
    exif_to_iso("", "-03:00", iso, sizeof iso);
    CHECK(iso[0] == 0);
    exif_to_short("", shrt, sizeof shrt);
    CHECK_STR(shrt, "--");

    if (failures) {
        printf("EXIF: %d falha(s)\n", failures);
        return 1;
    }
    printf("EXIF: todos os casos passaram\n");
    return 0;
}
