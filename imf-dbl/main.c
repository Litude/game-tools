#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "imf.h"

#define OUTPUT_PREFIX "DBL_"

imfFile* readFile (char *name) {
    printf("Processing file %s\n", name);
    FILE *file = fopen(name, "r");
    if (file) {
        fseek(file, 0L, SEEK_END);
        uint32_t fileSize = ftell(file);
        fseek(file, 0L, SEEK_SET);
        if (!fileSize % 4) {
            printf("File size not divisible by 4, not a valid IMF file\n");
            fclose(file);
            return NULL;
        }
        uint16_t entryCount = (uint16_t)(fileSize / 4);
        imfFile* entry = createImfFromStream(entryCount, file);
        fclose(file);
        return entry;

    } else {
        printf("Could not open file %s\n", name);
        return NULL;
    }

}

bool writeFile(imfFile* fileEntry, char* name) {
    FILE *file = fopen(name, "w");
    if (file) {
        printf("Writing file %s\n", name);
        bool success = writeImfToStream(fileEntry, file);
        fclose(file);
        return success;
    }
}

int main (uint32_t argc, char *argv[]) {
    char outputNameBuffer[255] = "";
    if (argc < 2) {
        printf("Usage: imfdbl FILE1 [FILE2] [FILE3]\n");
        return EXIT_FAILURE;
    } else {
        for (uint32_t i = 1; i < argc; ++i) {
            imfFile* fileEntry = readFile(argv[i]);
            if (fileEntry) {
                doubleImfDelays(fileEntry);
                strcpy(outputNameBuffer, OUTPUT_PREFIX);
                strcat(outputNameBuffer, argv[i]);
                writeFile(fileEntry, outputNameBuffer);
                freeImfFile(fileEntry);
            }
        }
        return EXIT_SUCCESS;
    }
}
