#ifndef KMGFXBLD_GRAPHIC_H
#define KMGFXBLD_GRAPHIC_H

#include <stdint.h>
#include <stdio.h>
#include "Palette.h"
#include "qdbmp_1.0.0/qdbmp.h"

typedef struct {
    int32_t x;
    int32_t y;
} POINT;

typedef struct {
    uint16_t width;
    uint16_t height;
    POINT offset;
    char* data;
} Graphic;

Graphic* Graphic_Create(uint16_t width, uint16_t height, char* data);
void Graphic_ClearFields(Graphic* gfx);
Graphic* Graphic_CreateFromBMP(BMP* bmpFile);
BMP* Graphic_ConvertToBMP(Graphic* gfx, Palette* palette);
Graphic* Graphic_ReadFromFile(FILE* filehandle);
void Graphic_WriteToFile(Graphic* gfx, FILE* filehandle);
void Graphic_Free(Graphic* gfx);

#endif