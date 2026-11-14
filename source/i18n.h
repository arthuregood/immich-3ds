#pragma once

// Textos da interface. Idioma vem da configuracao do console (CFGU):
// portugues se o 3DS estiver em portugues, ingles em qualquer outro caso.
typedef enum {
    S_SEND_THIS,
    S_SEND_DAY,      // "%d" = quantidade, "%s" = data
    S_CANCEL,
    S_OPTIONS,
    S_NO_DATE,
    S_PHOTOS,
    S_NO_PHOTOS,
    S_HINT,
    S_SETUP,         // dica quando falta servidor/chave
    S_NO_SERVER,
    S_SENDING,       // "%d" de "%d"
    S_RESULT,        // enviadas / repetidas / falhas
    S_BACK,
    S_EMPTY,         // nenhuma foto no cartao
    S_CHECKING,
    S_ONLINE,        // "%s" = versao do servidor
    S_OFFLINE,
    S_COUNT
} StrId;

void i18n_init(void);
const char *tr(StrId id);
