#include "page.h"
#include <string.h>

void sb_init(SBuf *b, char *buf, size_t cap)
{
    b->p = buf;
    b->n = 0;
    b->cap = cap;
    if (cap) buf[0] = 0;
}

void sb_str(SBuf *b, const char *s)
{
    size_t l = strlen(s);
    if (b->n + l >= b->cap) l = b->cap - b->n - 1;
    memcpy(b->p + b->n, s, l);
    b->n += l;
    b->p[b->n] = 0;
}

void sb_html(SBuf *b, const char *s)
{
    char c[2] = {0};
    for (; s && *s; s++) {
        switch (*s) {
            case '<': sb_str(b, "&lt;"); break;
            case '>': sb_str(b, "&gt;"); break;
            case '&': sb_str(b, "&amp;"); break;
            case '"': sb_str(b, "&quot;"); break;
            default:  c[0] = *s; sb_str(b, c); break;
        }
    }
}

// Textos da pagina. Tabela local: page.c precisa compilar no host (sem 3ds.h).
typedef struct { const char *pt, *en; } Tr;
static const char *t(int en, Tr x) { return en ? x.en : x.pt; }

#define T(en, pt, eng) t(en, (Tr){ pt, eng })

static void page_head(SBuf *b, int en)
{
    sb_str(b, "<!doctype html><meta charset=utf-8>"
              "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
              "<meta name=theme-color content=\"#0e1015\">"
              "<title>Immich 3DS</title><style>"
              "*{box-sizing:border-box}"
              "body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;"
              "padding:20px;background:#0e1015;color:#e7e9ee;"
              "font:14px/1.45 -apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif}"
              ".card{width:100%;max-width:380px;background:#161a21;border:1px solid #232935;"
              "border-radius:18px;padding:22px}"
              ".top{display:flex;align-items:center;gap:10px}"
              ".mark{width:26px;height:26px;border-radius:50%;flex:0 0 auto;"
              "background:conic-gradient(#e8453c 0 20%,#f0a30a 0 40%,#22a565 0 60%,"
              "#3478f6 0 80%,#e8519a 0 100%)}"
              "h1{font-size:15px;font-weight:600;margin:0;letter-spacing:.2px}"
              "p.sub{color:#77808f;font-size:12px;margin:4px 0 18px 36px}"
              "label{display:block;font-size:10px;letter-spacing:.09em;text-transform:uppercase;"
              "color:#78828f;margin:14px 0 6px}"
              "input{width:100%;height:38px;padding:0 11px;font-size:14px;color:#e7e9ee;"
              "background:#0e1015;border:1px solid #2a3140;border-radius:10px;outline:none}"
              "input:focus{border-color:#e8453c;box-shadow:0 0 0 3px rgba(232,69,60,.15)}"
              "input::placeholder{color:#4d5563}"
              "button{width:100%;height:40px;margin-top:20px;border:0;border-radius:10px;"
              "background:#e8453c;color:#fff;font-size:14px;font-weight:600;cursor:pointer}"
              "button:active{background:#cc3a32}"
              ".hint{color:#5b636f;font-size:11px;margin:14px 0 0}"
              ".msg{padding:14px;border-radius:12px;font-size:13px}"
              ".ok{background:#12261a;border:1px solid #24503a;color:#b8e6c8}"
              ".err{background:#2a1416;border:1px solid #5c2a2c;color:#f0c2c2}"
              ".mono{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:11px;"
              "color:#77808f;word-break:break-all}"
              "</style>");
    (void)en;
}

void page_form(SBuf *b, const Config *c, int en)
{
    page_head(b, en);
    sb_str(b, "<div class=card><div class=top><div class=mark></div><h1>Immich 3DS</h1></div>"
              "<p class=sub>");
    sb_str(b, T(en, "Configure e volte ao console.", "Set it up, then go back to the console."));
    sb_str(b, "</p><form method=post action=/save><label>");
    sb_str(b, T(en, "Servidor", "Server"));
    sb_str(b, "</label><input name=server inputmode=url autocapitalize=none autocorrect=off "
              "spellcheck=false value=\"");
    sb_html(b, c->server);
    sb_str(b, "\" placeholder=\"http://192.168.1.10:2283\"><label>API key</label>"
              "<input name=apikey autocapitalize=none autocorrect=off spellcheck=false value=\"");
    sb_html(b, c->apikey);
    sb_str(b, "\" placeholder=\"");
    sb_str(b, T(en, "cole a chave do Immich", "paste your Immich key"));
    sb_str(b, "\"><label>");
    sb_str(b, T(en, "Fuso", "Timezone"));
    sb_str(b, "</label><input name=tz value=\"");
    sb_html(b, c->tz);
    sb_str(b, "\"><button>");
    sb_str(b, T(en, "Enviar pro 3DS", "Send to 3DS"));
    sb_str(b, "</button></form><p class=hint>");
    sb_str(b, T(en,
               "Gere a chave em Account Settings &rarr; API Keys. Salva em sdmc:/immich-3ds/.env",
               "Create the key in Account Settings &rarr; API Keys. Saved to sdmc:/immich-3ds/.env"));
    sb_str(b, "</p></div>");
}

void page_done(SBuf *b, int ok, const char *extra, int en)
{
    page_head(b, en);
    sb_str(b, "<div class=card><div class=top><div class=mark></div><h1>Immich 3DS</h1></div>"
              "<p class=sub>");
    sb_str(b, ok ? T(en, "Configuracao recebida", "Configuration received")
                 : T(en, "Faltou dado", "Missing info"));
    sb_str(b, "</p>");
    sb_str(b, ok ? "<div class=\"msg ok\">" : "<div class=\"msg err\">");
    sb_str(b, ok ? T(en, "Salvo. Pode voltar ao 3DS &mdash; ele ja esta testando a conexao.",
                          "Saved. Go back to the 3DS &mdash; it is already testing the connection.")
                 : T(en, "Informe o servidor e a API key.", "Fill in the server and the API key."));
    sb_str(b, "</div>");
    if (extra && extra[0]) {
        sb_str(b, "<p class=mono>");
        sb_html(b, extra);
        sb_str(b, "</p>");
    }
    sb_str(b, "</div>");
}
