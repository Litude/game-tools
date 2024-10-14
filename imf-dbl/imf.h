#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t adlibReg;
    uint8_t data;
    uint16_t delay;
} imfData;
#pragma pack(pop)

typedef struct {
    uint16_t length;
    imfData* data;
} imfFile;

imfFile* createImfFromStream(uint16_t entryCount, FILE* stream);
bool writeImfToStream(imfFile* file, FILE* stream);
imfFile* doubleImfDelays(imfFile* file);
void freeImfFile(imfFile* file);
