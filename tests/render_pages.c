// Gera as paginas do pareamento como HTML, para conferir o visual no navegador.
// Roda no host: nao depende de 3ds.h.
#include <stdio.h>
#include <string.h>
#include "page.h"

int main(void)
{
    char buf[8192];
    SBuf b;
    Config c;
    memset(&c, 0, sizeof c);
    snprintf(c.server, sizeof c.server, "http://192.168.0.10:2283");
    snprintf(c.tz, sizeof c.tz, "-03:00");

    sb_init(&b, buf, sizeof buf);
    page_form(&b, &c, 0);
    FILE *f = fopen("/tmp/immich-form.html", "wb");
    fwrite(buf, 1, b.n, f);
    fclose(f);
    printf("form: %zu bytes\n", b.n);

    sb_init(&b, buf, sizeof buf);
    page_done(&b, 1, "http://192.168.0.10:2283", 0);
    f = fopen("/tmp/immich-done.html", "wb");
    fwrite(buf, 1, b.n, f);
    fclose(f);
    printf("done: %zu bytes\n", b.n);

    sb_init(&b, buf, sizeof buf);
    page_form(&b, &c, 1);
    f = fopen("/tmp/immich-form-en.html", "wb");
    fwrite(buf, 1, b.n, f);
    fclose(f);
    printf("form-en: %zu bytes\n", b.n);
    return 0;
}
