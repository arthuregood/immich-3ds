#include "config.h"
#include "config_build.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <3ds.h>

#define CONFIG_DIR  "sdmc:/immich-3ds"
#define CONFIG_FILE "sdmc:/immich-3ds/config.txt"
#define CA_FILE     "sdmc:/immich-3ds/ca.pem"

static void trim(char *s)
{
    char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = 0;
}

static bool file_exists(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0;
}

static void config_defaults(Config *c)
{
    memset(c, 0, sizeof *c);
    snprintf(c->server, sizeof c->server, "%s", BUILD_SERVER);
    snprintf(c->apikey, sizeof c->apikey, "%s", BUILD_APIKEY);
    snprintf(c->tz,     sizeof c->tz,     "%s", BUILD_TZ[0] ? BUILD_TZ : "-03:00");
}

bool config_has_defaults(void)
{
    return BUILD_SERVER[0] && BUILD_APIKEY[0];
}

bool config_load(Config *c)
{
    config_defaults(c);

    FILE *f = fopen(CONFIG_FILE, "r");
    if (f) {
        char line[420];
        while (fgets(line, sizeof line, f)) {
            trim(line);
            if (line[0] == '#' || !line[0]) continue;
            char *eq = strchr(line, '=');
            if (!eq) continue;
            *eq = 0;
            char *k = line, *v = eq + 1;
            trim(k); trim(v);
            if      (!strcasecmp(k, "server")) snprintf(c->server, sizeof c->server, "%s", v);
            else if (!strcasecmp(k, "apikey")) snprintf(c->apikey, sizeof c->apikey, "%s", v);
            else if (!strcasecmp(k, "tz"))     snprintf(c->tz,     sizeof c->tz,     "%s", v);
        }
        fclose(f);
    }

    // tira a barra final do server para concatenar "/api/..." sem "//"
    size_t sl = strlen(c->server);
    while (sl && c->server[sl - 1] == '/') c->server[--sl] = 0;

    c->verify = file_exists(CA_FILE);
    return c->server[0] && c->apikey[0];
}

bool config_save(const Config *c)
{
    mkdir(CONFIG_DIR, 0777);
    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) return false;
    fprintf(f, "server=%s\napikey=%s\ntz=%s\n", c->server, c->apikey, c->tz);
    fclose(f);
    return true;
}

static bool ask(const char *hint, const char *def, char *out, size_t outsz, bool secret)
{
    SwkbdState sw;
    swkbdInit(&sw, SWKBD_TYPE_NORMAL, 2, outsz - 1);
    swkbdSetHintText(&sw, hint);
    if (def && def[0]) swkbdSetInitialText(&sw, def);
    if (secret) swkbdSetPasswordMode(&sw, SWKBD_PASSWORD_HIDE_DELAY);
    char tmp[256] = {0};
    if (swkbdInputText(&sw, tmp, sizeof tmp) != SWKBD_BUTTON_RIGHT) return false;
    snprintf(out, outsz, "%s", tmp);
    trim(out);
    return out[0] != 0;
}

void config_setup(Config *c)
{
    if (!ask("Servidor (ex: http://192.168.0.10:2283)", c->server, c->server, sizeof c->server, false)) return;
    if (!ask("API key do Immich", c->apikey, c->apikey, sizeof c->apikey, true)) return;
    if (!ask("Fuso horario", c->tz, c->tz, sizeof c->tz, false)) return;
    size_t sl = strlen(c->server);
    while (sl && c->server[sl - 1] == '/') c->server[--sl] = 0;
    config_save(c);
    c->verify = file_exists(CA_FILE);
}
