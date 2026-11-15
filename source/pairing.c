#include "pairing.h"
#include "form.h"
#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>

#define PAIR_PORT 8080

static int  g_srv = -1;
static char g_url[48] = "";

static char g_req[2048];
static char g_page[6144];

//---------------------------------------------------------------------------------
typedef struct { char *p; size_t n, cap; } SBuf;

static void app(SBuf *b, const char *s)
{
    size_t l = strlen(s);
    if (b->n + l >= b->cap) l = b->cap - b->n - 1;
    memcpy(b->p + b->n, s, l);
    b->n += l;
    b->p[b->n] = 0;
}

static void app_html(SBuf *b, const char *s)   // escapa o minimo para nao quebrar o HTML
{
    char tmp[3] = {0};
    for (; *s; s++) {
        switch (*s) {
            case '<': app(b, "&lt;"); break;
            case '>': app(b, "&gt;"); break;
            case '&': app(b, "&amp;"); break;
            case '"': app(b, "&quot;"); break;
            default: tmp[0] = *s; app(b, tmp); break;
        }
    }
}

static void page_form(SBuf *b, const Config *c)
{
    app(b, "<!doctype html><meta charset=utf-8>"
           "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
           "<title>Immich 3DS</title><style>"
           "body{background:#15151f;color:#eee;font:16px system-ui;margin:0;padding:22px}"
           "h1{font-size:19px;color:#f9442c;margin:0 0 4px}"
           "p.sub{color:#888;font-size:13px;margin:0 0 18px}"
           "label{display:block;margin:16px 0 6px;color:#aaa;font-size:13px;text-transform:uppercase}"
           "input{width:100%;box-sizing:border-box;padding:12px;border-radius:8px;"
           "border:1px solid #333;background:#1f1f2b;color:#fff;font-size:16px}"
           "button{margin-top:22px;width:100%;padding:14px;border:0;border-radius:8px;"
           "background:#f9442c;color:#fff;font-size:16px;font-weight:600}"
           ".ok{background:#1f3a24;border:1px solid #3d7a48;padding:14px;border-radius:8px}"
           "</style><h1>Immich 3DS</h1><p class=sub>Configure e volte ao console.</p>"
           "<form method=post action=/save>"
           "<label>Servidor</label><input name=server value=\"");
    app_html(b, c->server);
    app(b, "\" placeholder=\"http://192.168.1.10:2283\">"
           "<label>API key</label><input name=apikey value=\"");
    app_html(b, c->apikey);
    app(b, "\" placeholder=\"Account Settings &rarr; API Keys\">"
           "<label>Fuso</label><input name=tz value=\"");
    app_html(b, c->tz);
    app(b, "\"><button>Enviar pro 3DS</button></form>"
           "<p class=sub style=\"margin-top:18px\">A key fica salva em sdmc:/immich-3ds/config.txt</p>");
}

static void page_done(SBuf *b, bool ok, const char *extra)
{
    app(b, "<!doctype html><meta charset=utf-8>"
           "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
           "<title>Immich 3DS</title><style>"
           "body{background:#15151f;color:#eee;font:16px system-ui;margin:0;padding:22px}"
           "h1{font-size:19px;color:#f9442c;margin:0 0 14px}"
           ".ok{background:#1f3a24;border:1px solid #3d7a48;padding:14px;border-radius:8px}"
           ".err{background:#3a1f1f;border:1px solid #7a3d3d;padding:14px;border-radius:8px}"
           "p{color:#aaa;font-size:14px}"
           "</style><h1>Immich 3DS</h1>");
    app(b, ok ? "<div class=ok>Salvo. Pode voltar ao 3DS.</div>"
                : "<div class=err>Faltou servidor ou API key.</div>");
    app(b, "<p>");
    app_html(b, extra ? extra : "");
    app(b, "</p>");
}

//---------------------------------------------------------------------------------
static void send_all(int fd, const char *s, size_t n)
{
    size_t off = 0;
    while (off < n) {
        int r = send(fd, s + off, n - off, 0);
        if (r <= 0) break;
        off += (size_t)r;
    }
}

static void respond(int fd, const char *body)
{
    char h[160];
    int n = snprintf(h, sizeof h,
                     "HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                     "Content-Length: %u\r\nConnection: close\r\n\r\n",
                     (unsigned)strlen(body));
    send_all(fd, h, (size_t)n);
    send_all(fd, body, strlen(body));
    closesocket(fd);
}

// Le o pedido inteiro (non-blocking com espera curta para nao travar o app).
static size_t read_request(int fd, size_t *body_off)
{
    size_t len = 0;
    g_req[0] = 0;
    *body_off = 0;

    for (int tries = 0; tries < 400; tries++) {
        fd_set r;
        FD_ZERO(&r);
        FD_SET(fd, &r);
        struct timeval tv = {0, 0};
        if (select(fd + 1, &r, NULL, NULL, &tv) > 0) {
            int got = recv(fd, g_req + len, sizeof g_req - 1 - len, 0);
            if (got <= 0) break;
            len += (size_t)got;
            g_req[len] = 0;

            char *hend = strstr(g_req, "\r\n\r\n");
            if (hend) {
                size_t hlen = (size_t)(hend - g_req);
                *body_off = hlen + 4;
                long cl = form_content_length(g_req, hlen);
                size_t have = len - *body_off;
                if (cl <= 0) return len;                       // GET: sem corpo
                if (have >= (size_t)cl) return len;
            }
        }
        svcSleepThread(2000000LL);                              // 2 ms
    }
    return len;
}

//---------------------------------------------------------------------------------
bool pairing_active(void) { return g_srv >= 0; }
const char *pairing_url(void) { return g_url; }

bool pairing_start(void)
{
    if (g_srv >= 0) return true;

    u32 ip = gethostid();                 // ordem de rede
    snprintf(g_url, sizeof g_url, "http://%u.%u.%u.%u:%d",
             (unsigned)(ip & 0xFF), (unsigned)((ip >> 8) & 0xFF),
             (unsigned)((ip >> 16) & 0xFF), (unsigned)((ip >> 24) & 0xFF), PAIR_PORT);

    g_srv = socket(AF_INET, SOCK_STREAM, 0);
    if (g_srv < 0) return false;

    int one = 1;
    setsockopt(g_srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_port = htons(PAIR_PORT);
    a.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_srv, (struct sockaddr *)&a, sizeof a) < 0) goto fail;
    if (listen(g_srv, 2) < 0) goto fail;

    return true;

fail:
    closesocket(g_srv);
    g_srv = -1;
    return false;
}

void pairing_stop(void)
{
    if (g_srv >= 0) closesocket(g_srv);
    g_srv = -1;
    g_url[0] = 0;
}

int pairing_poll(Config *cfg)
{
    if (g_srv < 0) return 0;

    fd_set r;
    FD_ZERO(&r);
    FD_SET(g_srv, &r);
    struct timeval tv = {0, 0};
    if (select(g_srv + 1, &r, NULL, NULL, &tv) <= 0) return 0;

    struct sockaddr_in cli;
    socklen_t cl = sizeof cli;
    int fd = accept(g_srv, (struct sockaddr *)&cli, &cl);
    if (fd < 0) return 0;

    size_t body_off = 0;
    size_t len = read_request(fd, &body_off);

    SBuf b = { g_page, 0, sizeof g_page };
    g_page[0] = 0;
    int ret = 0;

    if (len >= 3 && !strncmp(g_req, "GET", 3)) {
        page_form(&b, cfg);
    } else if (len >= body_off) {
        const char *body = g_req + body_off;
        char server[200] = {0}, apikey[200] = {0}, tz[16] = {0};
        form_get(body, len - body_off, "server", server, sizeof server);
        form_get(body, len - body_off, "apikey", apikey, sizeof apikey);
        form_get(body, len - body_off, "tz",     tz,     sizeof tz);

        if (server[0] && apikey[0]) {
            size_t sl = strlen(server);
            while (sl && server[sl - 1] == '/') server[--sl] = 0;
            snprintf(cfg->server, sizeof cfg->server, "%s", server);
            snprintf(cfg->apikey, sizeof cfg->apikey, "%s", apikey);
            if (tz[0]) snprintf(cfg->tz, sizeof cfg->tz, "%s", tz);
            page_done(&b, true, server);
            ret = 1;
        } else {
            page_done(&b, false, NULL);
        }
    }

    if (!b.n) page_done(&b, false, NULL);
    respond(fd, g_page);
    return ret;
}
