#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <curl/curl.h>
#include "config.h"
#include "dcim.h"
#include "exif.h"
#include "immich.h"

#define MAX_PHOTOS 512
#define LIST_ROWS  21
#define MAX_RESULTS 96

static PrintConsole topScreen, bottomScreen;
static Config cfg;
static Photo photos[MAX_PHOTOS];
static int nphotos, cursor, scroll;
static char results[MAX_RESULTS][78];
static int nresults;
static volatile int g_pct = -1;

static const char *host_of(const char *server)
{
    const char *p = strstr(server, "://");
    return p ? p + 3 : server;
}

static void draw_bottom(const char *msg)
{
    consoleSelect(&bottomScreen);
    printf("\x1b[2J");
    printf("\x1b[1;1H %s\n", host_of(cfg.server[0] ? cfg.server : "(sem servidor)"));
    printf("\n %d fotos  |  %d marcadas\n", nphotos, dcim_marked(photos, nphotos));
    if (!cfg.verify) printf("\n \x1b[33mTLS sem verificacao\x1b[0m\n (ca.pem ausente)");
    printf("\n\n %s\n", msg ? msg : "");
    printf("\n \x1b[30;47m START \x1b[0m enviar  \x1b[30;47m SELECT \x1b[0m config");
}

static void draw_list(const char *msg)
{
    consoleSelect(&topScreen);
    printf("\x1b[2J\x1b[1;1H");
    printf(" Immich 3DS  -  %d/%d fotos\n", dcim_marked(photos, nphotos), nphotos);
    printf(" -------------------------------------------------- \n");
    for (int i = scroll; i < nphotos && i < scroll + LIST_ROWS; i++) {
        char dt[24];
        exif_to_short(photos[i].datetime, dt, sizeof dt);
        printf("%c[%c] %-12s %4uK %s\n",
               i == cursor ? '>' : ' ', photos[i].marked ? 'x' : ' ',
               photos[i].name, photos[i].size / 1024, dt);
    }
    printf(" -------------------------------------------------- \n");
    printf(" A marca  Y todas  X limpa  START envia  B sai\n");
    draw_bottom(msg);
}

static void on_progress(double f)
{
    int pct = (int)(f * 100.0);
    if (pct == g_pct) return;
    g_pct = pct;
    char bar[41];
    int fill = pct * 40 / 100;
    memset(bar, ' ', 40);
    bar[40] = 0;
    for (int i = 0; i < fill; i++) bar[i] = '#';
    printf("\x1b[3;1H [%s] %3d%%", bar, pct);
    gfxFlushBuffers();
    gfxSwapBuffers();
}

static void upload_marked(void)
{
    int total = dcim_marked(photos, nphotos);
    nresults = 0;
    if (!total) { draw_list("nada marcado"); return; }

    int n = 0, ok = 0, dup = 0, fail = 0;
    for (int i = 0; i < nphotos; i++) {
        if (!photos[i].marked) continue;
        n++;

        consoleSelect(&topScreen);
        printf("\x1b[2J\x1b[1;1H");
        printf(" Enviando %d/%d\n %s\n", n, total, photos[i].name);
        printf("\n\n");
        g_pct = -1;
        on_progress(0);

        char status[24] = {0}, err[140] = {0};
        int r = immich_upload(&cfg, &photos[i], status, sizeof status, err, sizeof err, on_progress);

        char *line = results[nresults < MAX_RESULTS ? nresults++ : MAX_RESULTS - 1];
        if (r == 0) {
            if (!strcmp(status, "duplicate")) { dup++; snprintf(line, 78, " = %s (ja existia)", photos[i].name); }
            else                               { ok++;  snprintf(line, 78, " + %s", photos[i].name); }
            photos[i].marked = false;
        } else {
            fail++;
            snprintf(line, 78, " ! %s: %s", photos[i].name, err);
        }

        consoleSelect(&topScreen);
        printf("\x1b[6;1H");
        for (int k = (nresults > 8 ? nresults - 8 : 0); k < nresults; k++) printf(" %s\n", results[k]);
        gfxFlushBuffers();
        gfxSwapBuffers();
    }

    char msg[64];
    snprintf(msg, sizeof msg, "%d enviadas, %d repetidas, %d falharam", ok, dup, fail);
    draw_list(msg);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    gfxInitDefault();
    consoleInit(GFX_TOP, &topScreen);
    consoleInit(GFX_BOTTOM, &bottomScreen);

    socInit((u32 *)memalign(0x1000, 0x40000), 0x40000);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    config_load(&cfg);
    nphotos = dcim_scan(photos, MAX_PHOTOS);

    const char *msg = NULL;
    if (!cfg.server[0] || !cfg.apikey[0]) {
        msg = "configure com SELECT";
    } else {
        char ver[32] = {0}, err[140] = {0};
        if (immich_about(&cfg, ver, sizeof ver, err, sizeof err) == 0) {
            static char okmsg[64];
            snprintf(okmsg, sizeof okmsg, "servidor ok (%s)", ver);
            msg = okmsg;
        } else {
            static char errmsg[96];
            snprintf(errmsg, sizeof errmsg, "sem conexao: %s", err);
            msg = errmsg;
        }
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 k = hidKeysDown();

        if (k & KEY_B) break;

        if (k & KEY_SELECT) {
            config_setup(&cfg);
            int n = dcim_scan(photos, MAX_PHOTOS);
            if (n) nphotos = n;
            cursor = scroll = 0;
            msg = "config salva";
        }
        if (nphotos) {
            if (k & KEY_DOWN && cursor < nphotos - 1) cursor++;
            if (k & KEY_UP && cursor > 0) cursor--;
            if (k & KEY_RIGHT) cursor = cursor + LIST_ROWS < nphotos ? cursor + LIST_ROWS : nphotos - 1;
            if (k & KEY_LEFT)  cursor = cursor - LIST_ROWS > 0 ? cursor - LIST_ROWS : 0;
            if (k & KEY_A) photos[cursor].marked = !photos[cursor].marked;
            if (k & KEY_Y) for (int i = 0; i < nphotos; i++) photos[i].marked = true;
            if (k & KEY_X) for (int i = 0; i < nphotos; i++) photos[i].marked = false;
        }
        if (k & KEY_START) {
            if (cfg.server[0] && cfg.apikey[0]) { upload_marked(); msg = "fim do envio"; }
            else msg = "configure com SELECT";
        }

        if (cursor < scroll) scroll = cursor;
        if (cursor >= scroll + LIST_ROWS) scroll = cursor - LIST_ROWS + 1;

        draw_list(msg);
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    curl_global_cleanup();
    socExit();
    gfxExit();
    return 0;
}
