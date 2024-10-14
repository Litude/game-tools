#include <stdio.h>
#include <stdlib.h>
#include "Palette.h"
#include "utils.h"

uint32_t findLBMChunk(char* buffer, char* chunk, uint32_t bufferSize) {
    uint32_t i = 0;
    while (i < (bufferSize - 4)) {
        if (buffer[i] == chunk[0] &&
        buffer[i+1] == chunk[1] &&
        buffer[i+2] == chunk[2] &&
        buffer[i+3] == chunk[3]) {
            return i + 8;
        }
        ++i;
    }
    return UINT32_MAX;
}

Palette* Palette_ReadFromLBM(FILE* filehandle) {

    uint32_t fileSize = getFileSize(filehandle);
    char* filebuffer = malloc(fileSize);
    
    fread(filebuffer, fileSize, 1, filehandle);
    uint32_t palPos = findLBMChunk(filebuffer, "CMAP", fileSize);

    if (palPos == UINT32_MAX) {
        fprintf(stderr, "Could not locate palette in LBM file!\n");
        return NULL;
    }

    Palette* palette = malloc(sizeof(Palette));

    Color* palColor = (Color*)&filebuffer[palPos];
    for (uint32_t i = 0; i < 256; ++i) {
        palette->paletteEntries[i] = *palColor;
        ++palColor;
    }
    free(filebuffer);
    return palette;
}

void Palette_Free(Palette* palette) {
    free(palette);
}