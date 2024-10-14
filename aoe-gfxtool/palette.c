#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "qdbmp.h"

static uint8_t* Palette_ReadRaw(const char* path) {
    FILE* file = fopen(path, "rb");
    uint8_t* palette = malloc(3 * 256);
    if (palette) {
        fread(palette, 1, 768, file);
    }
    fclose(file);
    return palette;
}

static uint8_t* Palette_ReadBMP(const char* path) {
    BMP* bmpFile = BMP_ReadFile(path);
    if (!bmpFile || bmpFile->Header.BitsPerPixel != 8 || bmpFile->Header.ColorsUsed != 256) {
        printf("Palette file %s not valid 8-bit 256 color BMP file!\n", path);
        return NULL;
    }
    uint8_t* palette = malloc(3 * 256);
    if (palette) {
        for (int32_t i = 0; i < 256; ++i) {
            palette[i * 3] = bmpFile->Palette[i * 4 + 2];
            palette[i * 3 + 1] = bmpFile->Palette[i * 4 + 1];
            palette[i * 3 + 2] = bmpFile->Palette[i * 4];
        }
    }
    return palette;
}

static uint8_t* Palette_ReadJASC(const char* path) {
    FILE* file = fopen(path, "r");
    char lineBuffer[255];
    if (!fgets(lineBuffer, 255, file)) {
        printf("Unexpected end of file while reading JASC palette\n", path);
        fclose(file);
        return NULL;
    }
    if (strcmp(lineBuffer, "JASC-PAL\n")) {
        printf("JASC palette file %s has invalid header!\n", path);
        fclose(file);
        return NULL;
    }
    if (!fgets(lineBuffer, 255, file)) {
        printf("Unexpected end of file while reading JASC palette\n", path);
        fclose(file);
        return NULL;
    }
    if (strcmp(lineBuffer, "0100\n")) {
        printf("JASC palette file %s has invalid version, 0100 expected!\n", path);
        fclose(file);
        return NULL;
    }
    if (!fgets(lineBuffer, 255, file)) {
        printf("Unexpected end of file while reading JASC palette\n", path);
        fclose(file);
        return NULL;
    }
    if (strcmp(lineBuffer, "256\n")) {
        printf("JASC palette file %s should have exactly 256 colors, got %s!\n", path, lineBuffer);
        fclose(file);
        return NULL;
    }
    uint8_t* palette = malloc(3 * 256);
    if (palette) {
        int32_t red, green, blue;
        for (int32_t i = 0; i < 256; ++i) {
            int32_t count = fscanf(file, "%d %d %d\n", &red, &green, &blue);
            if (count != 3 || red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || green > 255) {
                printf("JASC palette file has invalid color entry");
                fclose(file);
                return NULL;
            }
            palette[i * 3] = (uint8_t)red;
            palette[i * 3 + 1] = (uint8_t)green;
            palette[i * 3 + 2] = (uint8_t)blue;
        }
    }
    fclose(file);
    return palette;
}

void Palette_Free(uint8_t* palette) {
    free(palette);
}

uint8_t* Palette_Create(const char* path) {
    FILE* paletteFile = fopen(path, "rb");
    if (!paletteFile) {
        printf("Error opening palette file %s\n", path);
        return NULL;
    }
    
    fseek(paletteFile, 0, SEEK_END);
    size_t paletteSize = ftell(paletteFile);
    fseek(paletteFile, 0, SEEK_SET);

    if (paletteSize == 768) {
        printf("Detected RAW palette\n");
        fclose(paletteFile);
        return Palette_ReadRaw(path);
    } else if (paletteSize > 8) {
        char headerBuffer[8];
        fread(headerBuffer, 1, 8, paletteFile);

        if (!memcmp(headerBuffer, "JASC-PAL", 8)) {
            printf("Detected JASC palette\n");
            return Palette_ReadJASC(path);

        } else if (!memcmp(headerBuffer, "BM", 2)) {
            printf("Detected BMP palette\n");
            return Palette_ReadBMP(path);
        }
    }
    printf("Unknown palette format\n");
    fclose(paletteFile);
    return NULL;
}
