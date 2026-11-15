#include "immich.h"
#include "exif.h"
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#define CA_FILE "sdmc:/immich-3ds/ca.pem"
#define BODY_MAX 1024

static char g_body[BODY_MAX + 1];
static size_t g_bodylen;

static size_t on_body(void *data, size_t sz, size_t n, void *ud)
{
    (void)ud;
    size_t len = sz * n;
    if (g_bodylen + len > BODY_MAX) len = BODY_MAX - g_bodylen;
    memcpy(g_body + g_bodylen, data, len);
    g_bodylen += len;
    g_body[BODY_MAX] = 0;
    return sz * n;                 // sempre reporta tudo, senao o curl aborta
}

static int xferinfo(void *ud, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    (void)dltotal; (void)dlnow;
    if (ultotal > 0 && ud) ((UploadProgress)ud)((double)ulnow / (double)ultotal);
    return 0;
}

static CURL *make_handle(const Config *c, char *err, size_t errsz, struct curl_slist **headers)
{
    CURL *h = curl_easy_init();
    if (!h) { snprintf(err, errsz, "curl_easy_init falhou"); return NULL; }

    char hdr[256];
    snprintf(hdr, sizeof hdr, "x-api-key: %s", c->apikey);
    *headers = curl_slist_append(*headers, hdr);
    *headers = curl_slist_append(*headers, "Expect:");          // sem 100-continue
    *headers = curl_slist_append(*headers, "Accept: application/json");
    curl_easy_setopt(h, CURLOPT_HTTPHEADER, *headers);
    curl_easy_setopt(h, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(h, CURLOPT_TIMEOUT, 600L);
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, on_body);
    curl_easy_setopt(h, CURLOPT_USERAGENT, "immich-3ds");
    curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);

    g_bodylen = 0;
    g_body[0] = 0;

    if (c->verify) {
        curl_easy_setopt(h, CURLOPT_CAINFO, CA_FILE);
        curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 2L);
    } else {
        curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 0L);        // sem ca.pem no cartao: nao valida o servidor
        curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    return h;
}

static int finish(CURL *h, struct curl_slist *headers, CURLcode rc, char *err, size_t errsz)
{
    long code = 0;
    curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(h);
    curl_slist_free_all(headers);
    if (rc != CURLE_OK) {
        snprintf(err, errsz, "%s", curl_easy_strerror(rc));
        return -1;
    }
    if (code < 200 || code > 299) {
        // o corpo do Immich traz a mensagem de erro em JSON; mostra cru, truncado
        snprintf(err, errsz, "HTTP %ld %s", code, g_body);
        return -2;
    }
    return 0;
}

int immich_about(const Config *c, char *version, size_t vsz, char *err, size_t errsz)
{
    struct curl_slist *headers = NULL;
    CURL *h = make_handle(c, err, errsz, &headers);
    if (!h) return -1;

    char url[256];
    snprintf(url, sizeof url, "%s/api/server/about", c->server);
    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, 4L);   // check de status: nao travar a UI
    curl_easy_setopt(h, CURLOPT_TIMEOUT, 6L);

    CURLcode rc = curl_easy_perform(h);
    int r = finish(h, headers, rc, err, errsz);
    if (r) return r;

    char *p = strstr(g_body, "\"version\"");
    if (version && vsz) {
        version[0] = 0;
        if (p) {
            char *q = strchr(p, ':'); if (q) q = strchr(q, '"');
            if (q) {
                char *e = strchr(q + 1, '"');
                if (e) snprintf(version, vsz, "%.*s", (int)(e - q - 1), q + 1);
            }
        }
    }
    return 0;
}

int immich_upload(const Config *c, const Photo *p, char *status, size_t statussz,
                  char *err, size_t errsz, UploadProgress prog)
{
    struct curl_slist *headers = NULL;
    CURL *h = make_handle(c, err, errsz, &headers);
    if (!h) return -1;

    char url[256], iso[40];
    snprintf(url, sizeof url, "%s/api/assets", c->server);
    if (p->datetime[0]) exif_to_iso(p->datetime, c->tz, iso, sizeof iso);
    else                snprintf(iso, sizeof iso, "1970-01-01T00:00:00.000Z");

    curl_mime *mime = curl_mime_init(h);
    curl_mimepart *part;

    part = curl_mime_addpart(mime);                 // streaming do arquivo, sem carregar na RAM
    curl_mime_name(part, "assetData");
    curl_mime_filedata(part, p->path);
    curl_mime_filename(part, p->name);
    curl_mime_type(part, "image/jpeg");

    part = curl_mime_addpart(mime); curl_mime_name(part, "filename");
    curl_mime_data(part, p->name, CURL_ZERO_TERMINATED);
    part = curl_mime_addpart(mime); curl_mime_name(part, "fileCreatedAt");
    curl_mime_data(part, iso, CURL_ZERO_TERMINATED);
    part = curl_mime_addpart(mime); curl_mime_name(part, "fileModifiedAt");
    curl_mime_data(part, iso, CURL_ZERO_TERMINATED);

    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_setopt(h, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(h, CURLOPT_XFERINFOFUNCTION, xferinfo);
    curl_easy_setopt(h, CURLOPT_XFERINFODATA, prog);
    curl_easy_setopt(h, CURLOPT_NOPROGRESS, 0L);

    CURLcode rc = curl_easy_perform(h);
    curl_mime_free(mime);
    int r = finish(h, headers, rc, err, errsz);
    if (r) return r;

    if (status && statussz) {                        // {"id":"...","status":"created"}
        status[0] = 0;
        char *p2 = strstr(g_body, "\"status\"");
        if (p2) {
            char *q = strchr(p2, ':');
            if (q) q = strchr(q, '"');
            if (q) {
                char *e = strchr(q + 1, '"');
                if (e) snprintf(status, statussz, "%.*s", (int)(e - q - 1), q + 1);
            }
        }
    }
    return 0;
}
