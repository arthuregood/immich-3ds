#pragma once
#include <stdbool.h>
#include <stddef.h>

// Percent-decoding de application/x-www-form-urlencoded ("+" vira espaco, "%41" vira 'A').
void form_decode(const char *src, size_t len, char *out, size_t outsz);

// Procura "key=valor" no corpo urlencoded e devolve o valor ja decodificado.
bool form_get(const char *body, size_t len, const char *key, char *out, size_t outsz);

// Le o Content-Length de um cabecalho HTTP. <0 se nao houver.
long form_content_length(const char *headers, size_t len);
