#include "pairing.h"
#include "form.h"
#include "page.h"
#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>

#define PAIR_PORT 8080

static int  g_srv = -1;
static int  g_en = 1;
static char g_url[48] = "";

static char g_req[2048];
static char g_page[6144];

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

// Le o pedido inteiro (non-blocking, com espera curta para nao travar o app).
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
void pairing_set_lang(int en) { g_en = en; }
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

    SBuf b;
    sb_init(&b, g_page, sizeof g_page);
    int ret = 0;

    if (len >= 3 && !strncmp(g_req, "GET", 3)) {
        page_form(&b, cfg, g_en);
    } else if (len >= body_off) {
        const char *body = g_req + body_off;
        size_t blen = len - body_off;
        char server[200] = {0}, apikey[200] = {0}, tz[16] = {0};
        form_get(body, blen, "server",   server, sizeof server);
        form_get(body, blen, "apikey",   apikey, sizeof apikey);
        form_get(body, blen, "tz",       tz,     sizeof tz);
        form_get(body, blen, "IMMICH_SERVER",  server, sizeof server);
        form_get(body, blen, "IMMICH_API_KEY", apikey, sizeof apikey);

        if (server[0] && apikey[0]) {
            size_t sl = strlen(server);
            while (sl && server[sl - 1] == '/') server[--sl] = 0;
            snprintf(cfg->server, sizeof cfg->server, "%s", server);
            snprintf(cfg->apikey, sizeof cfg->apikey, "%s", apikey);
            if (tz[0]) snprintf(cfg->tz, sizeof cfg->tz, "%s", tz);
            page_done(&b, 1, server, g_en);
            ret = 1;
        } else {
            page_done(&b, 0, NULL, g_en);
        }
    }

    if (!b.n) page_done(&b, 0, NULL, g_en);
    respond(fd, g_page);
    return ret;
}
