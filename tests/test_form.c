// Teste do parser de formulario urlencoded (pareamento). Roda no host.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "form.h"

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
    char out[256];

    // percent-decoding
    form_decode("a%41b+c%2Fd", strlen("a%41b+c%2Fd"), out, sizeof out);
    CHECK_STR(out, "aAb c/d");

    form_decode("100%", 4, out, sizeof out);          // '%' solto no fim nao quebra
    CHECK_STR(out, "100%");

    form_decode("", 0, out, sizeof out);
    CHECK_STR(out, "");

    // campos de um POST real do formulario
    const char *body = "server=http%3A%2F%2F1.2.3.4%3A2283&apikey=abcDEF123&tz=-03%3A00";
    size_t blen = strlen(body);

    CHECK(form_get(body, blen, "server", out, sizeof out));
    CHECK_STR(out, "http://1.2.3.4:2283");

    CHECK(form_get(body, blen, "apikey", out, sizeof out));
    CHECK_STR(out, "abcDEF123");

    CHECK(form_get(body, blen, "tz", out, sizeof out));
    CHECK_STR(out, "-03:00");

    // chave ausente
    CHECK(!form_get(body, blen, "lang", out, sizeof out));

    // chave que e' prefixo de outra nao pode casar
    CHECK(!form_get("apikeyextra=1&apikey=ok", strlen("apikeyextra=1&apikey=ok"), "apikey", out, sizeof out) == 0);
    CHECK(form_get("apikeyextra=1&apikey=ok", strlen("apikeyextra=1&apikey=ok"), "apikey", out, sizeof out));
    CHECK_STR(out, "ok");

    // Content-Length
    const char *hdr = "POST /save HTTP/1.1\r\nHost: x\r\nContent-Length: 42\r\n\r\n";
    CHECK(form_content_length(hdr, strlen(hdr)) == 42);
    CHECK(form_content_length("GET / HTTP/1.1\r\n\r\n", 18) < 0);

    if (failures) { printf("FORM: %d falha(s)\n", failures); return 1; }
    printf("FORM: todos os casos passaram\n");
    return 0;
}
