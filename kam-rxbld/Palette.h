#ifndef KMGFXBLD_PALETTE_H
#define KMGFXBLD_PALETTE_H

#include <stdint.h>

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Color;

typedef struct {
    Color paletteEntries[256];
} Palette;

Palette* Palette_ReadFromLBM(FILE* filehandle);
void Palette_Free(Palette* palette);

#endif
