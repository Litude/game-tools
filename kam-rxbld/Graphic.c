#include <stdlib.h>
#include <string.h>
#include "Graphic.h"

Graphic* Graphic_Create(uint16_t width, uint16_t height, char* data) {
    Graphic* gfx = malloc(sizeof(Graphic));
    Graphic_ClearFields(gfx);
    gfx->data = malloc(width * height);
    if (gfx->data) {
        if (data) {
            memcpy(gfx->data, data, height * width);
        } else {
            memset(gfx->data, 0, height * width);
        }
        gfx->width = width;
        gfx->height = height;
    }
    return gfx;
}

Graphic* Graphic_CreateFromBMP(BMP* bmpFile) {
    if (!(BMP_GetDepth(bmpFile) == 8)) {
        fprintf(stderr, "BMP is not 8-bit palletted, skipping file...");
        return NULL;
    }
    uint16_t width = BMP_GetWidth(bmpFile);
    uint16_t height = BMP_GetHeight(bmpFile);

    char* pixelData = malloc(width * height);

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            BMP_GetPixelIndex(bmpFile, x, y, &pixelData[y*width+x]);
        }
    }
    Graphic* gfx = Graphic_Create(width, height, pixelData);
    free(pixelData);
    return gfx;
}

BMP* Graphic_ConvertToBMP(Graphic* gfx, Palette* palette) {
    BMP* bmpFile = BMP_Create(gfx->width, gfx->height, 8);
	//printf("Creating %dx%d BMP file...\n", gfx->width, gfx->height);
    for (uint32_t j = 0; j < 256; ++j) {
        Color curColor = palette->paletteEntries[j];
        BMP_SetPaletteColor(bmpFile, j, curColor.red, curColor.green, curColor.blue);
    }

    for (uint32_t y = 0; y < gfx->height; ++y) {
        for (uint32_t x = 0; x < gfx->width; ++x) {
            BMP_SetPixelIndex(bmpFile, x, y, gfx->data[y*gfx->width+x]);
        }
    }
    return bmpFile;
}

void Graphic_ClearFields(Graphic* gfx) {
    gfx->width = 0;
    gfx->height = 0;
    gfx->offset.x = 0;
    gfx->offset.y = 0;
    gfx->data = NULL;
}

Graphic* Graphic_ReadFromFile(FILE* filehandle) {
    Graphic gfxBuffer;
    fread(&gfxBuffer.width, 2, 1, filehandle);
    fread(&gfxBuffer.height, 2, 1, filehandle);
	fread(&gfxBuffer.offset.x, 4, 1, filehandle);
	fread(&gfxBuffer.offset.y, 4, 1, filehandle);

    Graphic* gfx = Graphic_Create(gfxBuffer.width, gfxBuffer.height, NULL);
    gfx->offset = gfxBuffer.offset;
    if (gfx->data) {
        fread(gfx->data, 1, gfx->width * gfx->height, filehandle);
    }
    return gfx;
}

void Graphic_WriteToFile(Graphic* gfx, FILE* filehandle) {
    fwrite(&gfx->width, sizeof(uint16_t), 1, filehandle);
    fwrite(&gfx->height, sizeof(uint16_t), 1, filehandle);
    fwrite(&gfx->offset.x, sizeof(int32_t), 1, filehandle);
    fwrite(&gfx->offset.y, sizeof(int32_t), 1, filehandle);
    fwrite(gfx->data, gfx->width*gfx->height, 1, filehandle);
}

void Graphic_Free(Graphic* gfx) {
    if (gfx->data) free(gfx->data);
    free(gfx);
}