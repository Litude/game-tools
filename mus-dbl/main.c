#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "mus.h"

#define OUTPUT_PREFIX "DBL_"

musFile* readFile (char *name) {
    printf("Processing file %s\n", name);
    FILE *file = fopen(name, "r");
    if (file) {
        fseek(file, 0L, SEEK_END);
        uint32_t fileSize = ftell(file);
        fseek(file, 0L, SEEK_SET);
        if (fileSize < 4) {
            printf("File size less than size of MUS header, not valid file\n");
            fclose(file);
            return NULL;
        }
        musFile* entry = createMusFromStream(file);
        fclose(file);
        return entry;

    } else {
        printf("Could not open file %s\n", name);
        return NULL;
    }

}

bool writeFile(musFile* fileEntry, char* name) {
    FILE *file = fopen(name, "w");
    if (file) {
        printf("Writing file %s\n", name);
        bool success = writeMusToStream(fileEntry, file);
        fclose(file);
        return success;
    }
}

int main (uint32_t argc, char *argv[]) {
    char outputNameBuffer[255] = "";
    if (argc < 2) {
        printf("Usage: musdbl FILE1 [FILE2] [FILE3]\n");
        return EXIT_FAILURE;
    } else {
        for (uint32_t i = 1; i < argc; ++i) {
            musFile* fileEntry = readFile(argv[i]);
            if (fileEntry) {
                doubleMusDelays(fileEntry);
                strcpy(outputNameBuffer, OUTPUT_PREFIX);
                strcat(outputNameBuffer, argv[i]);
                writeFile(fileEntry, outputNameBuffer);
                freeMusFile(fileEntry);
            }
        }
        return EXIT_SUCCESS;
    }
}
