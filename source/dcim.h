#pragma once
#include <stdbool.h>

typedef struct {
    char path[200];
    char name[40];
    unsigned int size;
    char datetime[20];   // "YYYY:MM:DD HH:MM:SS" ou vazio
    bool marked;
} Photo;

// Varre sdmc:/DCIM/<subpasta>/*.JPG e *.MPO. Devolve a quantidade encontrada.
int dcim_scan(Photo *out, int max);
int dcim_marked(const Photo *p, int n);
