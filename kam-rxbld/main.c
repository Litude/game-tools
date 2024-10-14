#include <string.h>
#include <stdlib.h>
#include "Graphic.h"
#include "GraphicIndex.h"
#include "Palette.h"

int main(int argc, char** argv) {

    printf("KaMRXBld v1.00\n");

    if (argc < 4) {
        printf("Usage:\n");
        printf("For exporting: %s -export resfile palettefile\n", argv[0]);
        printf("For importing: %s -import resfile numGraphics\n", argv[0]);
        return 1;
    }

    if (!strcmp(argv[1], "-export")) {
        printf("Exporting %s by using palette file %s\n", argv[2], argv[3]);

        FILE* gfxHandle = NULL;
        FILE* palHandle = NULL;

        if (!(gfxHandle = fopen(argv[2], "rb"))) {
            fprintf(stderr, "Error opening file %s, quitting...\n", argv[2]);
            return EXIT_FAILURE;
        }
        printf("Parsing graphics file %s...\n", argv[2]);
        GraphicIndex* gfxData = GraphicIndex_ReadFromFile(gfxHandle);
        fclose(gfxHandle);

        if (!(palHandle = fopen(argv[3], "rb"))) {
            fprintf(stderr, "Error opening file %s, quitting...\n", argv[3]);
            GraphicIndex_Free(gfxData);
            return EXIT_FAILURE;
        }
        printf("Parsing palette file %s...\n", argv[3]);
        Palette* palData = Palette_ReadFromLBM(palHandle);
        fclose(palHandle);

        printf("Graphics entries in file: %d\n", gfxData->numEntries);

        printf("Writing BMP files...\n");
        GraphicIndex_WriteToBMPFiles(gfxData, palData, "output");

        Palette_Free(palData);
        GraphicIndex_Free(gfxData);


    } else if (!strcmp(argv[1], "-import")) {
        printf("Creating %s with %s entries\n", argv[2], argv[3]);
        FILE* rxHandle = NULL;
        char* validEntries = NULL;

        if (!(rxHandle = fopen(argv[2], "wb"))) {
            fprintf(stderr, "Error opening file %s, quitting...\n", argv[2]);
            return EXIT_FAILURE;
        }

        int32_t numEntries = (int32_t)strtol(argv[3], NULL, 10);

        printf("Reading BMP files...\n");
		GraphicIndex* gfxIndex = GraphicIndex_ReadFromBMPFiles("input", numEntries);

        printf("Writing RX file...\n");
        GraphicIndex_WriteToFile(gfxIndex, rxHandle);

        GraphicIndex_Free(gfxIndex);
        fclose(rxHandle);
        
    } else {
        printf("Unknown 2nd argument, must be either -export or -import\n");
    }


    return EXIT_SUCCESS;
}