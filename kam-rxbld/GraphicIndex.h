#ifndef KMGFXBLD_GRAPHICINDEX_H
#define KMGFXBLD_GRAPHICINDEX_H

#include <stdint.h>
#include "Graphic.h"

typedef struct {
    Graphic** entryData;
    int32_t numEntries;
} GraphicIndex;

GraphicIndex* GraphicIndex_Create(uint32_t numEntries);
void GraphicIndex_ClearFields(GraphicIndex* gfxRes);
GraphicIndex* GraphicIndex_ReadFromFile(FILE* filehandle);
void GraphicIndex_WriteToFile(GraphicIndex* gfxRes, FILE* fileHandle);
GraphicIndex* GraphicIndex_ReadFromBMPFiles(const char* directory, int32_t numEntries);
void GraphicIndex_WriteToBMPFiles(GraphicIndex* gfxIndex, Palette* palData, const char* directory);
void GraphicIndex_Free(GraphicIndex* gfxRes);

#endif
