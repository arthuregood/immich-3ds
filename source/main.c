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
#include "font.h"
#include "i18n.h"
#include "pairing.h"

#define TOP_W 400
#define TOP_H 240
#define BOT_W 320
#define BOT_H 240

// grade 3x2 na tela de baixo
#define COLS    3
#define ROWS    2
#define PAGE    (COLS * ROWS)
#define CELL_W  106
#define CELL_H  80
#define GRID_Y  18
#define HEADER_H GRID_Y
#define FOOTER_Y (GRID_Y + ROWS * CELL_H)

// preview na tela de cima (4:3, com barra de status embaixo)
#define PREV_W  288
#define PREV_H  216
#define PREV_X  ((TOP_W - PREV_W) / 2)
#define STATUS_Y 216

#define MAX_PHOTOS 512

enum { ST_GALLERY, ST_MENU, ST_UPLOAD, ST_DONE, ST_PAIRING };
enum { MENU_PHOTO, MENU_SETTINGS };

static Config cfg;
static Photo photos[MAX_PHOTOS];
static int nphotos;
static int cursor, page;
static int state = ST_GALLERY;
static int menu_sel;
static int menu_kind;
static bool sent[MAX_PHOTOS];      // ja enviada nesta sessao (so para marcar na grade)
static bool dirty = true;

static uint8_t thumb_px[PAGE][CELL_W * CELL_H * 3];
static int     thumb_key[PAGE];
static uint8_t prev_px[PREV_W * PREV_H * 3];
static int     prev_key = -1;

static int g_online = -1;         // -1 checando, 0 offline, 1 online
static char g_version[24];

static int g_up_done, g_up_total, g_pct;
static char g_up_name[40];
static char g_result[96];
static int  g_ok, g_dup, g_fail;

//---------------------------------------------------------------------------------
static int bpp_of(GSPGPU_FramebufferFormat f)
{
    switch (f) {
        case GSP_BGR8_OES:  return 3;
        case GSP_RGBA8_OES: return 4;
        default:            return 2;
    }
}

static const char *host_of(const char *server)
{
    const char *p = strstr(server, "://");
    return p ? p + 3 : server;
}

// "2026:11:13 22:15:23" -> "13/11/2026"
static void day_of(const Photo *p, char *out, size_t n)
{
    if (!p || strlen(p->datetime) < 10) { snprintf(out, n, "%s", tr(S_NO_DATE)); return; }
    snprintf(out, n, "%.2s/%.2s/%.4s", p->datetime + 8, p->datetime + 5, p->datetime);
}

// "2026:11:13 22:15:23" -> "22:15"
static void time_of(const Photo *p, char *out, size_t n)
{
    if (!p || strlen(p->datetime) < 16) { snprintf(out, n, "--:--"); return; }
    snprintf(out, n, "%.5s", p->datetime + 11);
}

static bool same_day(const Photo *a, const Photo *b)
{
    return !strncmp(a->datetime, b->datetime, 10);
}

static int count_day(int idx)
{
    int n = 0;
    for (int i = 0; i < nphotos; i++)
        if (same_day(&photos[i], &photos[idx])) n++;
    return n;
}

//---------------------------------------------------------------------------------
static void draw_top(void)
{
    int bpp = bpp_of(gfxGetScreenFormat(GFX_TOP));
    uint8_t *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    if (!fb) return;

    thumb_fill(fb, bpp, 0, 0, TOP_W, TOP_H, TOP_W, TOP_H, 0x0C, 0x0C, 0x12);

    if (nphotos) {
        if (prev_key != cursor) {
            thumb_decode_rgb(photos[cursor].path, PREV_W, PREV_H, prev_px);
            prev_key = cursor;
        }
        thumb_blit(prev_px, PREV_W, PREV_H, fb, bpp, PREV_X, 0, TOP_W, TOP_H);
    }

    char line[64], t[8];
    time_of(nphotos ? &photos[cursor] : NULL, t, sizeof t);
    if (nphotos)
        snprintf(line, sizeof line, "%s  %s", photos[cursor].name, t);
    else
        snprintf(line, sizeof line, "%s", tr(S_EMPTY));

    thumb_fill(fb, bpp, 0, STATUS_Y, TOP_W, TOP_H - STATUS_Y, TOP_W, TOP_H, 0x18, 0x18, 0x24);
    font_draw(fb, bpp, 6, STATUS_Y + 8, line, TOP_W, TOP_H, 0xE8, 0xE8, 0xF0);
}

static void draw_grid(uint8_t *fb, int bpp)
{
    for (int k = 0; k < PAGE; k++) {
        int i = page + k;
        int dx = (k % COLS) * CELL_W;
        int dy = GRID_Y + (k / COLS) * CELL_H;

        if (i >= nphotos) {
            thumb_fill(fb, bpp, dx, dy, CELL_W, CELL_H, BOT_W, BOT_H, 0x14, 0x14, 0x1C);
            continue;
        }
        if (thumb_key[k] != i) {
            if (!thumb_decode_rgb(photos[i].path, CELL_W, CELL_H, thumb_px[k]))
                memset(thumb_px[k], 0x20, sizeof thumb_px[k]);
            thumb_key[k] = i;
        }
        thumb_blit(thumb_px[k], CELL_W, CELL_H, fb, bpp, dx, dy, BOT_W, BOT_H);

        // divisor quando muda o dia
        if (i > 0 && !same_day(&photos[i], &photos[i - 1]))
            thumb_fill(fb, bpp, dx, dy, 1, CELL_H, BOT_W, BOT_H, 0xF0, 0xD0, 0x40);

        if (sent[i])                                   // ja enviada: barra verde
            thumb_fill(fb, bpp, dx + CELL_W - 14, dy + 3, 11, 4, BOT_W, BOT_H, 0x50, 0xC8, 0x60);

        if (i == cursor)                               // selecionada
            thumb_border(fb, bpp, dx, dy, CELL_W, CELL_H, 2, BOT_W, BOT_H, 255, 255, 255);
    }
}

// Itens do menu: depende de onde ele foi aberto (foto ou configuracao).
static void menu_items(const char **items, char *b1, size_t b1sz, char *b2, size_t b2sz)
{
    if (menu_kind == MENU_SETTINGS) {
        items[0] = tr(S_SET_BROWSER);
        items[1] = tr(S_SET_KEYBOARD);
        items[2] = tr(S_CANCEL);
        return;
    }
    char day[16];
    day_of(nphotos ? &photos[cursor] : NULL, day, sizeof day);
    snprintf(b1, b1sz, "%s", tr(S_SEND_THIS));
    snprintf(b2, b2sz, tr(S_SEND_DAY), nphotos ? count_day(cursor) : 0, day);
    items[0] = b1;
    items[1] = b2;
    items[2] = tr(S_CANCEL);
}

static void draw_menu(uint8_t *fb, int bpp)
{
    char item1[40], item2[64];
    const char *items[3];
    menu_items(items, item1, sizeof item1, item2, sizeof item2);
    int bx = 18, by = 62, bw = BOT_W - 36, bh = 26 + 3 * 20;

    thumb_fill(fb, bpp, bx - 2, by - 2, bw + 4, bh + 4, BOT_W, BOT_H, 0xFF, 0xFF, 0xFF);
    thumb_fill(fb, bpp, bx, by, bw, bh, BOT_W, BOT_H, 0x1A, 0x1A, 0x28);
    font_draw(fb, bpp, bx + 8, by + 5, tr(S_OPTIONS), BOT_W, BOT_H, 0x90, 0x90, 0xB0);

    for (int i = 0; i < 3; i++) {
        int y = by + 22 + i * 20;
        if (i == menu_sel)
            thumb_fill(fb, bpp, bx + 2, y - 2, bw - 4, 14, BOT_W, BOT_H, 0x30, 0x50, 0x90);
        font_draw(fb, bpp, bx + 10, y, items[i], BOT_W, BOT_H, 0xF0, 0xF0, 0xF8);
    }
}

static void draw_bottom(void)
{
    int bpp = bpp_of(gfxGetScreenFormat(GFX_BOTTOM));
    uint8_t *fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
    if (!fb) return;

    thumb_fill(fb, bpp, 0, 0, BOT_W, BOT_H, BOT_W, BOT_H, 0x0C, 0x0C, 0x12);
    draw_grid(fb, bpp);

    // cabecalho: dia da foto atual
    char day[16], hdr[64];
    if (nphotos) {
        day_of(&photos[cursor], day, sizeof day);
        snprintf(hdr, sizeof hdr, "%s  %d %s", day, count_day(cursor), tr(S_PHOTOS));
    } else {
        snprintf(hdr, sizeof hdr, "%s", tr(S_NO_PHOTOS));
    }
    font_draw(fb, bpp, 4, 4, hdr, BOT_W, BOT_H, 0xF0, 0xD0, 0x40);

    char pg[24];
    snprintf(pg, sizeof pg, "%d-%d/%d", page + 1,
             (page + PAGE > nphotos ? nphotos : page + PAGE), nphotos);
    font_draw(fb, bpp, BOT_W - 4 - font_width(pg), 4, pg, BOT_W, BOT_H, 0x90, 0x90, 0xA8);

    // rodape
    thumb_fill(fb, bpp, 0, FOOTER_Y, BOT_W, BOT_H - FOOTER_Y, BOT_W, BOT_H, 0x14, 0x14, 0x1E);
    char l2[64], t[8];
    time_of(nphotos ? &photos[cursor] : NULL, t, sizeof t);
    snprintf(l2, sizeof l2, "%s", nphotos ? photos[cursor].name : "-");
    font_draw(fb, bpp, 4, FOOTER_Y + 4, l2, BOT_W, BOT_H, 0xE0, 0xE0, 0xE8);
    font_draw(fb, bpp, BOT_W - 4 - font_width(t), FOOTER_Y + 4, t, BOT_W, BOT_H, 0xE0, 0xE0, 0xE8);
    font_draw(fb, bpp, 4, FOOTER_Y + 16, tr(S_HINT), BOT_W, BOT_H, 0x88, 0x88, 0xA0);
    font_draw(fb, bpp, 4, FOOTER_Y + 28, cfg.server[0] ? host_of(cfg.server) : tr(S_NO_SERVER),
              BOT_W, BOT_H, 0x70, 0x70, 0x88);
    char st[48] = {0};
    if (!cfg.server[0] || !cfg.apikey[0]) st[0] = 0;
    else if (g_online < 0) snprintf(st, sizeof st, "%s", tr(S_CHECKING));
    else if (g_online == 0) snprintf(st, sizeof st, "%s", tr(S_OFFLINE));
    else                    snprintf(st, sizeof st, tr(S_ONLINE), g_version);
    if (st[0])
    font_draw(fb, bpp, BOT_W - 4 - font_width(st), FOOTER_Y + 28, st, BOT_W, BOT_H,
              g_online == 1 ? 0x60 : (g_online == 0 ? 0xE0 : 0x90),
              g_online == 1 ? 0xC8 : (g_online == 0 ? 0x60 : 0x90),
              g_online == 1 ? 0x60 : (g_online == 0 ? 0x60 : 0xA0));
    if (state == ST_MENU) draw_menu(fb, bpp);
}

static void draw_upload_popup(void)
{
    int bpp = bpp_of(gfxGetScreenFormat(GFX_TOP));
    uint8_t *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    if (!fb) return;

    int bx = 24, by = 70, bw = TOP_W - 48, bh = 100;
    thumb_fill(fb, bpp, bx, by, bw, bh, TOP_W, TOP_H, 0x18, 0x18, 0x26);
    thumb_border(fb, bpp, bx, by, bw, bh, 1, TOP_W, TOP_H, 0x90, 0x90, 0xB0);

    char l[48];
    snprintf(l, sizeof l, tr(S_SENDING), g_up_done, g_up_total);
    font_draw(fb, bpp, bx + 10, by + 12, l, TOP_W, TOP_H, 0xF0, 0xF0, 0xF8);
    font_draw(fb, bpp, bx + 10, by + 32, g_up_name, TOP_W, TOP_H, 0xA8, 0xA8, 0xC0);

    int barw = bw - 20, fill = barw * g_pct / 100;
    thumb_fill(fb, bpp, bx + 10, by + 58, barw, 12, TOP_W, TOP_H, 0x28, 0x28, 0x38);
    thumb_fill(fb, bpp, bx + 10, by + 58, fill, 12, TOP_W, TOP_H, 0x50, 0x90, 0xE0);

    snprintf(l, sizeof l, "%d%%", g_pct);
    font_draw(fb, bpp, bx + 10, by + 78, l, TOP_W, TOP_H, 0xC0, 0xC0, 0xD0);
}

static void draw_result_popup(void)
{
    int bpp = bpp_of(gfxGetScreenFormat(GFX_TOP));
    uint8_t *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    if (!fb) return;

    int bx = 20, by = 80, bw = TOP_W - 40, bh = 80;
    thumb_fill(fb, bpp, bx, by, bw, bh, TOP_W, TOP_H, 0x18, 0x18, 0x26);
    thumb_border(fb, bpp, bx, by, bw, bh, 1, TOP_W, TOP_H, 0x90, 0x90, 0xB0);
    font_draw(fb, bpp, bx + 10, by + 16, g_result, TOP_W, TOP_H, 0xF0, 0xF0, 0xF8);
    font_draw(fb, bpp, bx + 10, by + 48, tr(S_BACK), TOP_W, TOP_H, 0x90, 0x90, 0xA8);
}

// Painel do servidor de pareamento (tela de cima).
static void draw_pairing(void)
{
    int bpp = bpp_of(gfxGetScreenFormat(GFX_TOP));
    uint8_t *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    if (!fb) return;

    int bx = 16, by = 56, bw = TOP_W - 32, bh = 128;
    thumb_fill(fb, bpp, bx, by, bw, bh, TOP_W, TOP_H, 0x18, 0x18, 0x26);
    thumb_border(fb, bpp, bx, by, bw, bh, 1, TOP_W, TOP_H, 0x90, 0x90, 0xB0);

    const char *t1 = tr(S_PAIR_TITLE);
    font_draw(fb, bpp, bx + (bw - font_width(t1)) / 2, by + 12, t1, TOP_W, TOP_H, 0xF0, 0xF0, 0xF8);

    if (pairing_active()) {
        const char *url = pairing_url();
        font_draw(fb, bpp, bx + (bw - font_width(url)) / 2, by + 40, url, TOP_W, TOP_H, 0xF9, 0x44, 0x2C);
        const char *t2 = tr(S_PAIR_HINT);
        font_draw(fb, bpp, bx + (bw - font_width(t2)) / 2, by + 66, t2, TOP_W, TOP_H, 0xA0, 0xA0, 0xB8);
        const char *t3 = tr(S_PAIR_WAIT);
        font_draw(fb, bpp, bx + (bw - font_width(t3)) / 2, by + 96, t3, TOP_W, TOP_H, 0x80, 0x80, 0x98);
    } else {
        const char *t2 = tr(S_PAIR_FAIL);
        font_draw(fb, bpp, bx + (bw - font_width(t2)) / 2, by + 56, t2, TOP_W, TOP_H, 0xE0, 0x70, 0x60);
        const char *t3 = tr(S_BACK);
        font_draw(fb, bpp, bx + (bw - font_width(t3)) / 2, by + 92, t3, TOP_W, TOP_H, 0x80, 0x80, 0x98);
    }
}

static void render(void)
{
    draw_top();
    draw_bottom();

    if (state == ST_UPLOAD) draw_upload_popup();
    if (state == ST_DONE)   draw_result_popup();
    if (state == ST_PAIRING) draw_pairing();

    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
    dirty = false;
}

//---------------------------------------------------------------------------------
static void on_progress(double f)
{
    int pct = (int)(f * 100.0);
    if (pct == g_pct) return;
    g_pct = pct;
    render();
}

static void send_list(const int *idx, int n)
{
    g_ok = g_dup = g_fail = 0;
    g_up_total = n;
    g_up_done = 0;
    state = ST_UPLOAD;

    for (int k = 0; k < n; k++) {
        int i = idx[k];
        snprintf(g_up_name, sizeof g_up_name, "%s", photos[i].name);
        g_pct = 0;
        g_up_done = k + 1;
        render();

        char status[24] = {0}, err[120] = {0};
        int r;
#ifdef DEMO
        // modo demo: progresso de mentira, sem tocar na rede
        for (int p = 0; p <= 100; p += 2) {
            g_pct = p;
            render();
            svcSleepThread(10000000LL);
        }
        snprintf(status, sizeof status, "created");
        r = 0;
#else
        r = immich_upload(&cfg, &photos[i], status, sizeof status, err, sizeof err, on_progress);
#endif
        if (r == 0) {
            if (!strcmp(status, "duplicate")) g_dup++;
            else { g_ok++; sent[i] = true; }
        } else {
            g_fail++;
        }
        g_pct = 100;
        render();
    }

    snprintf(g_result, sizeof g_result, tr(S_RESULT), g_ok, g_dup, g_fail);
    state = ST_DONE;
    dirty = true;
}

static void send_current(void)
{
    int idx[1] = { cursor };
    send_list(idx, 1);
}

static void send_day(void)
{
    int idx[MAX_PHOTOS], n = 0;
    for (int i = 0; i < nphotos && n < MAX_PHOTOS; i++)
        if (same_day(&photos[i], &photos[cursor])) idx[n++] = i;
    if (n) send_list(idx, n);
}

//---------------------------------------------------------------------------------
// Fala com o servidor sem travar a UI: timeout curto, e o resultado vai para o rodape.
static void check_status(void)
{
#ifdef DEMO
    g_online = 1;                       // no GIF nao existe rede: mostra o estado bom
    snprintf(g_version, sizeof g_version, "v3.4.1");
    dirty = true;
    return;
#endif
    g_online = -1;
    g_version[0] = 0;
    dirty = true;
    if (!cfg.server[0] || !cfg.apikey[0]) { g_online = 0; return; }

    char err[100] = {0};
    g_online = (immich_about(&cfg, g_version, sizeof g_version, err, sizeof err) == 0) ? 1 : 0;
    dirty = true;
}

// Executa o item escolhido no menu (foto ou configuracao).
static void menu_activate(void)
{
    state = ST_GALLERY;
    dirty = true;

    if (menu_kind == MENU_SETTINGS) {
        if (menu_sel == 0) {
            pairing_start();
            state = ST_PAIRING;
        } else if (menu_sel == 1) {
            config_setup(&cfg);
            i18n_init();
            pairing_set_lang(i18n_is_en());
            check_status();
        }
        return;
    }

    if (menu_sel == 0) send_current();
    else if (menu_sel == 1) send_day();
}

static void clamp_page(void)
{
    if (cursor < page) page = (cursor / PAGE) * PAGE;
    if (cursor >= page + PAGE) page = (cursor / PAGE) * PAGE;
    if (page >= nphotos) page = ((nphotos - 1) / PAGE) * PAGE;
    if (page < 0) page = 0;
}

static void handle_touch(void)
{
    touchPosition t;
    hidTouchRead(&t);

    if (state == ST_MENU) {
        int bx = 18, by = 62;
        for (int i = 0; i < 3; i++) {
            int y = by + 22 + i * 20;
            if (t.px >= bx && t.px < bx + (BOT_W - 36) && t.py >= y - 2 && t.py < y + 14) {
                menu_sel = i;
                menu_activate();
                return;
            }
        }
        state = ST_GALLERY;
        dirty = true;
        return;
    }

    if (state != ST_GALLERY) { state = ST_GALLERY; dirty = true; return; }

    int cx = t.px / CELL_W, cy = (t.py - GRID_Y) / CELL_H;
    if (t.py >= GRID_Y && t.py < FOOTER_Y && cx < COLS && cy < ROWS) {
        int i = page + cy * COLS + cx;
        if (i < nphotos) {
            if (i == cursor) { state = ST_MENU; menu_sel = 0; }
            else cursor = i;
            dirty = true;
        }
    }
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_TOP, true);
    gfxSetDoubleBuffering(GFX_BOTTOM, true);

    socInit((u32 *)memalign(0x1000, 0x40000), 0x40000);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    i18n_init();
    pairing_set_lang(i18n_is_en());
    config_load(&cfg);
    nphotos = dcim_scan(photos, MAX_PHOTOS);
    for (int i = 0; i < PAGE; i++) thumb_key[i] = -1;

    if (!cfg.server[0] || !cfg.apikey[0]) {
        if (pairing_start()) state = ST_PAIRING;    // primeira execucao: configura pelo navegador
    }

    render();        // mostra a tela antes de ir na rede
    if (state != ST_PAIRING) check_status();

#ifdef DEMO
    static u32 demo_n, demo_done_n;
    u32 status_n = 0;
#endif

    while (aptMainLoop()) {
        hidScanInput();
        u32 k = hidKeysDown();

#ifdef DEMO
        // roteiro para capturar o GIF: navega, abre o menu, envia, volta
        demo_n++;
        status_n = demo_n;
        if (state == ST_GALLERY) {
            if (status_n == 175 || status_n == 230 || status_n == 285 || status_n == 340)
                k |= KEY_DOWN;
            if (status_n == 410) k |= KEY_A;          // abre o menu
        } else if (state == ST_MENU) {
            if (status_n == 480) k |= KEY_DOWN;       // vai para "enviar as N do dia"
            if (status_n == 550) k |= KEY_A;          // confirma
        } else if (state == ST_DONE) {
            if (!demo_done_n) demo_done_n = status_n;
            if (status_n > demo_done_n + 110) { k |= KEY_A; demo_done_n = 0; }
        }
#endif

        if (k & KEY_SELECT) {
            state = ST_MENU;
            menu_kind = MENU_SETTINGS;
            menu_sel = 0;
            dirty = true;
        }

        if (state == ST_GALLERY) {
            if (k & KEY_B) break;
            if (nphotos) {
                if ((k & KEY_RIGHT) && cursor < nphotos - 1) { cursor++; dirty = true; }
                if ((k & KEY_LEFT) && cursor > 0) { cursor--; dirty = true; }
                if ((k & KEY_DOWN) && cursor + COLS < nphotos) { cursor += COLS; dirty = true; }
                if ((k & KEY_UP) && cursor - COLS >= 0) { cursor -= COLS; dirty = true; }
                if (k & KEY_A) { state = ST_MENU; menu_kind = MENU_PHOTO; menu_sel = 0; dirty = true; }
            }
            clamp_page();
        } else if (state == ST_MENU) {
            if (k & KEY_B) { state = ST_GALLERY; dirty = true; }
            if (k & KEY_DOWN) { menu_sel = (menu_sel + 1) % 3; dirty = true; }
            if (k & KEY_UP) { menu_sel = (menu_sel + 2) % 3; dirty = true; }
            if (k & KEY_A) menu_activate();
        } else if (state == ST_DONE) {
            if (k & (KEY_A | KEY_B)) { state = ST_GALLERY; dirty = true; }
        } else if (state == ST_PAIRING) {
            if (k & KEY_B) { pairing_stop(); state = ST_GALLERY; dirty = true; }
            Config tmp = cfg;
            if (pairing_poll(&tmp) == 1) {
                cfg = tmp;
                config_save(&cfg);
                pairing_stop();
                state = ST_GALLERY;
                dirty = true;
                render();
                check_status();
            }
        }

        if (k & KEY_TOUCH) handle_touch();

        if (dirty) render();
        else gspWaitForVBlank();
    }

    curl_global_cleanup();
    socExit();
    gfxExit();
    return 0;
}
