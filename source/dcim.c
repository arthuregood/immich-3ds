#include "dcim.h"
#include "exif.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define DCIM_ROOT "sdmc:/DCIM"

static bool is_photo(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot) return false;
    return !strcasecmp(dot, ".jpg") || !strcasecmp(dot, ".mpo");
}

static int scan_dir(const char *dirpath, Photo *out, int n, int max)
{
    DIR *d = opendir(dirpath);
    if (!d) return n;
    struct dirent *e;
    while ((e = readdir(d)) && n < max) {
        if (e->d_name[0] == '.') continue;
        if (!is_photo(e->d_name)) continue;

        Photo *p = &out[n];
        memset(p, 0, sizeof *p);
        snprintf(p->path, sizeof p->path, "%s/%s", dirpath, e->d_name);
        snprintf(p->name, sizeof p->name, "%s", e->d_name);

        struct stat st;
        if (stat(p->path, &st) == 0) p->size = (unsigned int)st.st_size;
        exif_datetime(p->path, p->datetime, sizeof p->datetime);
        n++;
    }
    closedir(d);
    return n;
}

// mais recente primeiro (string "YYYY:MM:DD HH:MM:SS" ordena lexicograficamente =
// cronologico, entao invertemos a comparacao). Sem data vai para o fim.
static int cmp_photo(const void *a, const void *b)
{
    const Photo *x = (const Photo *)a, *y = (const Photo *)b;
    int c = strcmp(y->datetime, x->datetime);
    return c ? c : strcmp(x->name, y->name);
}

int dcim_scan(Photo *out, int max)
{
    int n = 0;
    DIR *d = opendir(DCIM_ROOT);
    if (!d) return 0;

    struct dirent *e;
    while ((e = readdir(d)) && n < max) {
        if (e->d_name[0] == '.') continue;
        char sub[220];
        snprintf(sub, sizeof sub, "%s/%s", DCIM_ROOT, e->d_name);
        struct stat st;
        if (stat(sub, &st) != 0 || !S_ISDIR(st.st_mode)) continue;
        n = scan_dir(sub, out, n, max);
    }
    closedir(d);

    qsort(out, n, sizeof *out, cmp_photo);   // agrupa por dia na galeria
    return n;
}

int dcim_marked(const Photo *p, int n)
{
    int c = 0;
    for (int i = 0; i < n; i++) if (p[i].marked) c++;
    return c;
}
