#include "i18n.h"
#include <3ds.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

static int g_en = 1;

static const char *tbl[2][S_COUNT] = {
    /* ingles */ {
        "Send this photo",
        "Send all %d from %s",
        "Cancel",
        "Options",
        "no date",
        "photos",
        "no photos",
        "A: options    SELECT: setup",
        "SELECT: setup",
        "no server",
        "Sending %d of %d",
        "%d uploaded, %d duplicate, %d failed",
        "A: back",
        "no photos in sdmc:/DCIM",
        "checking...",
        "online %s",
        "offline",
    },
    /* portugues */ {
        "Enviar esta foto",
        "Enviar as %d de %s",
        "Cancelar",
        "Opcoes",
        "sem data",
        "fotos",
        "sem fotos",
        "A: opcoes    SELECT: config",
        "SELECT: configurar",
        "sem servidor",
        "Enviando %d de %d",
        "%d enviadas, %d repetidas, %d falhas",
        "A: voltar",
        "nenhuma foto em sdmc:/DCIM",
        "checando...",
        "online %s",
        "offline",
    },
};

void i18n_init(void)
{
    // IMMICH_LANG=pt|en em sdmc:/immich-3ds/config.txt vence o idioma do console
    FILE *f = fopen("sdmc:/immich-3ds/config.txt", "r");
    if (f) {
        char line[128];
        while (fgets(line, sizeof line, f)) {
            char *eq = strchr(line, '=');
            if (!eq) continue;
            *eq = 0;
            if (strcasecmp(line, "lang")) continue;
            char *v = eq + 1;
            while (*v == ' ') v++;
            if (!strncasecmp(v, "pt", 2)) { g_en = 0; fclose(f); return; }
            if (!strncasecmp(v, "en", 2)) { g_en = 1; fclose(f); return; }
        }
        fclose(f);
    }

    u8 lang = CFG_LANGUAGE_EN;
    if (R_FAILED(CFGU_GetSystemLanguage(&lang))) return;
    g_en = (lang != CFG_LANGUAGE_PT);
}

const char *tr(StrId id)
{
    if (id < 0 || id >= S_COUNT) return "";
    return tbl[g_en ? 0 : 1][id];
}
