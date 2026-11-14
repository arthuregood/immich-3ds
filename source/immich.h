#pragma once
#include <stddef.h>
#include "config.h"
#include "dcim.h"

typedef void (*UploadProgress)(double fraction);

// 0 = enviado (ou ja existia -> status "duplicate"). <0 = erro, detalhe em err.
int immich_upload(const Config *c, const Photo *p, char *status, size_t statussz,
                  char *err, size_t errsz, UploadProgress prog);

// 0 = servidor respondeu (version em out). <0 = falhou.
int immich_about(const Config *c, char *version, size_t vsz, char *err, size_t errsz);
