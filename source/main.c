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
#include "thumb.h"

#define MAX_PHOTOS  512
#define LIST_ROWS   21
#define MAX_RESULTS 96

// grade de thumbnails na tela de baixo: 3 colunas x 2 linhas
#define GRID_COLS 3
#define GRID_ROWS 2
#define GRID_N    (GRID_COLS * GRID_ROWS)
#define TH_W      106
#define TH_H      80
#define GRID_Y    48                     // y inicial da grade (6 linhas de texto acima)

static PrintConsole topScreen, bottomScreen;
static Config cfg;
static Photo photos[MAX_PHOTOS];
static int nphotos, cursor, scroll;
static char results[MAX_RESULTS][78];
static int nresults;
static volatile int g_pct = -1;

static uint8_t thumb_px[GRID_N][TH_W * TH_H * 3];
static int thumb_idx[GRID_N];            // qual foto esta em cada celula (-1 = vazia)

// libctru guarda o framebuffer no consoleInit; com double buffering precisamos
// reapontar a cada swap, senao o console escreve no buffer que nao esta na tela.
static void sync_console_fb(void)
{
    topScreen.frameBuffer    = (u16 *)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    bottomScreen.frameBuffer = (u16 *)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
}

static const char *host_of(const char *server)
{
    const char *p = strstr(server, "://");
    return p ? p + 3 : server;
}

// Quais fotos mostrar: as marcadas (o lote que vai subir) ou, se nada marcado,
// as proximas do cursor.
static int grid_indices(int *idx)
{
    int n = 0;
    for (int i = 0; i < nphotos && n < GRID_N; i++) if (photos[i].marked) idx[n++] = i;
    if (n) return n;
    for (int i = cursor; i < nphotos && n < GRID_N; i++) idx[n++] = i;
    return n;
}

static void draw_thumbs(void)
{
    int idx[GRID_N];
    int n = grid_indices(idx);

    u16 fw = 0, fh = 0;
    uint8_t *fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &fw, &fh);
    if (!fb || !fw || !fh) return;

    for (int k = 0; k < GRID_N; k++) {
        int dx = (k % GRID_COLS) * TH_W;
        int dy = GRID_Y + (k / GRID_COLS) * TH_H;

        if (k >= n) {
            // celula vazia: apaga o que sobrou da selecao anterior
            if (thumb_idx[k] != -1) {
                memset(thumb_px[k], 0x10, sizeof thumb_px[k]);
                thumb_idx[k] = -1;
            }
        } else if (thumb_idx[k] != idx[k]) {
            thumb_decode_rgb(photos[idx[k]].path, TH_W, TH_H, thumb_px[k]);
            thumb_idx[k] = idx[k];
        }
        thumb_blit_bgr(thumb_px[k], TH_W, TH_H, fb, fw * 3, dx, dy, fw, fh);

        if (k < n && idx[k] == cursor)
            thumb_border(fb, fw * 3, dx, dy, TH_W, TH_H, 2, fw, fh, 255, 255, 255);
    }
}

static void draw_bottom(const char *msg)
{
    consoleSelect(&bottomScreen);
    consoleClear();                       // 40x30 a tela toda: por isso o texto fica no topo
    printf(" %s%s\n", host_of(cfg.server[0] ? cfg.server : "(sem servidor)"),
           config_has_defaults() ? " (build)" : "");
    printf(" %d fotos | %d marcadas\n", nphotos, dcim_marked(photos, nphotos));
    // so 6 linhas: abaixo disso comeca a grade de thumbnails
    printf(" TLS %s\n", cfg.verify ? "verificado" : "\x1b[33msem verificacao\x1b[0m");
    printf(" %s\n", msg ? msg : "");
    printf(" A marca  START envia  SELECT config");
    printf("\n");
}

static void draw_list(const char *msg)
{
    consoleSelect(&topScreen);
    consoleClear();
    printf(" Immich 3DS  -  %d/%d fotos\n", dcim_marked(photos, nphotos), nphotos);
    printf(" --------------------------------------------------\n");
    for (int i = scroll; i < nphotos && i < scroll + LIST_ROWS; i++) {
        char dt[24];
        exif_to_short(photos[i].datetime, dt, sizeof dt);
        printf("%c[%c] %-12s %4uK %s\n",
               i == cursor ? '>' : ' ', photos[i].marked ? 'x' : ' ',
               photos[i].name, photos[i].size / 1024, dt);
    }
    printf(" --------------------------------------------------\n");
    printf(" A marca  Y todas  X limpa  B sai\n");
    draw_bottom(msg);
    draw_thumbs();
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
    printf("\x1b[4;1H [%s] %3d%%", bar, pct);
    gfxFlushBuffers();
    gfxSwapBuffers();
    sync_console_fb();
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
        consoleClear();
        printf(" Enviando %d/%d\n %s\n\n", n, total, photos[i].name);
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
        sync_console_fb();
    }

    char msg[64];
    snprintf(msg, sizeof msg, "%d enviadas, %d repetidas, %d falhas", ok, dup, fail);
    draw_list(msg);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    gfxInitDefault();
    consoleInit(GFX_TOP, &topScreen);
    consoleInit(GFX_BOTTOM, &bottomScreen);
    sync_console_fb();

    // limpa os dois framebuffers uma vez: a grade de thumbnails nao cobre a tela toda
    for (int scr = 0; scr <= 1; scr++) {
        u16 fw = 0, fh = 0;
        uint8_t *fb = gfxGetFramebuffer((gfxScreen_t)scr, GFX_LEFT, &fw, &fh);
        if (fb && fw && fh) memset(fb, 0, (size_t)fw * fh * 3);
    }


    socInit((u32 *)memalign(0x1000, 0x40000), 0x40000);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    config_load(&cfg);
    nphotos = dcim_scan(photos, MAX_PHOTOS);
    for (int i = 0; i < GRID_N; i++) { thumb_idx[i] = -1; memset(thumb_px[i], 0x10, sizeof thumb_px[i]); }

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
            for (int i = 0; i < GRID_N; i++) thumb_idx[i] = -1;
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
        sync_console_fb();
        gspWaitForVBlank();
    }

    curl_global_cleanup();
    socExit();
    gfxExit();
    return 0;
}
