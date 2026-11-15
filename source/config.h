#pragma once
#include <stdbool.h>
#include <stddef.h>

// Lido de sdmc:/immich-3ds/.env  (nunca fica no repositório)
typedef struct {
    char server[200];   // ex: http://192.168.0.10:2283  (sem barra no fim)
    char apikey[200];
    char tz[12];        // ex: -03:00
    bool verify;        // true se sdmc:/immich-3ds/ca.pem existir
} Config;

bool config_load(Config *c);
bool config_save(const Config *c);
void config_setup(Config *c);   // teclado do sistema (swkbd)

// true quando o build embutiu servidor + chave via .env
bool config_has_defaults(void);
