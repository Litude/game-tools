#include <stdlib.h>
#include "imf.h"

imfFile* createImfFromStream(uint16_t entryCount, FILE* stream) {
    imfFile* entry = (imfFile*)malloc(sizeof(imfFile));
    if (entry) {
        entry->length = entryCount;
        entry->data = (imfData*)malloc(entryCount * sizeof(imfData));
        if (entry->data) {
            fread(entry->data, sizeof(imfData), entryCount, stream);
            return entry;
        }
        free(entry);
    }
    return NULL;
};

bool writeImfToStream(imfFile* file, FILE* stream) {
    if (fwrite(file->data, sizeof(imfData), file->length, stream) == file->length) {
        return true;
    } else {
        return false;
    }
};

imfFile* doubleImfDelays(imfFile* file) {
    for (uint32_t i = 0; i < file->length; ++i) {
        file->data[i].delay *= 2;
    }
    return file;
}

void freeImfFile(imfFile* file) {
    free(file->data);
    free(file);
}
