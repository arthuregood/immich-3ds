#pragma once
#include <stdbool.h>
#include "config.h"

// Servidor HTTP de pareamento: o usuario abre a URL no celular/PC,
// digita servidor + chave, e o 3DS recebe. Sem teclado no console.
void pairing_set_lang(int en);
bool pairing_start(void);
void pairing_stop(void);
bool pairing_active(void);
const char *pairing_url(void);

// Chamar no loop principal. 0 = nada, 1 = recebeu e preencheu cfg, <0 = erro.
int pairing_poll(Config *cfg);
