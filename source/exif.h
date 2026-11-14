#pragma once
#include <stdbool.h>
#include <stddef.h>

// Le so o DateTime (0x0132) ou DateTimeOriginal (0x9003) do APP1/EXIF.
// Devolve "YYYY:MM:DD HH:MM:SS" ou "" se nao achar.
bool exif_datetime(const char *path, char *out, size_t outsz);

// "YYYY:MM:DD HH:MM:SS" -> "YYYY-MM-DDTHH:MM:SS.000<tz>"
void exif_to_iso(const char *exif, const char *tz, char *out, size_t outsz);

// "YYYY:MM:DD HH:MM:SS" -> "YYYY-MM-DD HH:MM" (para a tela)
void exif_to_short(const char *exif, char *out, size_t outsz);
