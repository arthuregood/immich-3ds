#pragma once
#include <stddef.h>
#include "config.h"

// Construtor de HTML do pareamento. Sem socket, sem 3ds.h: da' para compilar
// no host e renderizar a pagina para conferir o visual.

typedef struct { char *p; size_t n, cap; } SBuf;

void sb_init(SBuf *b, char *buf, size_t cap);
void sb_str(SBuf *b, const char *s);
void sb_html(SBuf *b, const char *s);      // escapa < > & "

// en != 0 => ingles (a pagina segue o idioma do console)
void page_form(SBuf *b, const Config *c, int en);
void page_done(SBuf *b, int ok, const char *extra, int en);
