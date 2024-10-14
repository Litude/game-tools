#include "Graphic.h"
#include "GraphicIndex.h"
#include "utils.h"
#include "qdbmp_1.0.0/qdbmp.h"
#include <direct.h>
#include <stdlib.h>

GraphicIndex* GraphicIndex_Create(uint32_t numEntries) {
    GraphicIndex* gfxRes = malloc(sizeof(GraphicIndex));
    gfxRes->numEntries = numEntries;
    GraphicIndex_ClearFields(gfxRes);
    gfxRes->entryData = malloc(sizeof(Graphic*) * gfxRes->numEntries);
    if (gfxRes->entryData) {
        for (uint32_t i = 0; i < gfxRes->numEntries; ++i) {
            gfxRes->entryData[i] = NULL;
        }
    }
    return gfxRes;

}

void GraphicIndex_ClearFields(GraphicIndex* gfxRes) {
    gfxRes->entryData = NULL;
}

GraphicIndex* GraphicIndex_ReadFromFile(FILE* filehandle) {
    uint32_t numEntries = 0;
    char* validEntries = NULL;

    fread(&numEntries, 4, 1, filehandle);
    if (numEntries) {
        GraphicIndex* gfxRes = GraphicIndex_Create(numEntries);
        validEntries = malloc(numEntries);

        for (uint32_t i = 0; i < gfxRes->numEntries; ++i) {
            fread(&validEntries[i], 1, 1, filehandle);
        }

        for (uint32_t i = 0; i < gfxRes->numEntries; ++i) {
            if (validEntries[i]) {
                gfxRes->entryData[i] = Graphic_ReadFromFile(filehandle);
            }
        }

        free(validEntries);

        return gfxRes;
    }
    return NULL;
}

void GraphicIndex_WriteToFile(GraphicIndex* gfxRes, FILE* fileHandle) {
    fwrite(&gfxRes->numEntries, sizeof(int32_t), 1, fileHandle);
    for (int32_t i = 0; i < gfxRes->numEntries; ++i) {
        uint8_t validEntry = gfxRes->entryData[i] != NULL;
        fwrite(&validEntry, sizeof(uint8_t), 1, fileHandle);
    }
    for (int32_t i = 0; i < gfxRes->numEntries; ++i) {
        if (gfxRes->entryData[i]) {
            Graphic_WriteToFile(gfxRes->entryData[i], fileHandle);
        }
    }
}

GraphicIndex* GraphicIndex_ReadFromBMPFiles(const char* directory, int32_t numEntries) {
	char namebase[100];
	char filename[100];

	GraphicIndex* gfxIndex = GraphicIndex_Create(numEntries);

	for (uint32_t i = 0; i < numEntries; ++i) {
		BMP* bmpFile = NULL;

		sprintf(namebase, "%s%cimg_%06u", directory, PATH_SEPARATOR, i);

		sprintf(filename, "%s.%s", namebase, "bmp");
		if (bmpFile = BMP_ReadFile(filename)) {
			gfxIndex->entryData[i] = Graphic_CreateFromBMP(bmpFile);

			if (gfxIndex->entryData[i]) {
				sprintf(filename, "%s.%s", namebase, "txt");
				FILE* offsetFile = fopen(filename, "rb");

				if (offsetFile) {
					fscanf(offsetFile, "%d;%d", &(gfxIndex->entryData[i]->offset.x), &(gfxIndex->entryData[i]->offset.y));
					fclose(offsetFile);
				}
			}
			BMP_Free(bmpFile);
		}
		else if (BMP_GetError() != BMP_FILE_NOT_FOUND) {
			fprintf(stderr, "BMP %06u error: %s\n", i, BMP_GetErrorDescription());
		}
	}

	return gfxIndex;
}

void GraphicIndex_WriteToBMPFiles(GraphicIndex* gfxIndex, Palette* palData, const char* directory) {
	char namebase[100];
	char filename[100];

	_mkdir(directory);

	for (uint32_t i = 0; i < gfxIndex->numEntries; ++i) {
		Graphic* gfxEntry = gfxIndex->entryData[i];
		if (!gfxEntry) continue;

		BMP* bmpFile = Graphic_ConvertToBMP(gfxEntry, palData);

		sprintf(namebase, "%s%cimg_%06u", directory, PATH_SEPARATOR, i);

		if (gfxEntry->offset.x || gfxEntry->offset.y) {
			sprintf(filename, "%s.%s", namebase, "txt");
			FILE* offsetFile = fopen(filename, "wb");
			if (offsetFile) {
				fprintf(offsetFile, "%d;%d", gfxEntry->offset.x, gfxEntry->offset.y);
				fclose(offsetFile);
			}
		}

		sprintf(filename, "%s.%s", namebase, "bmp");
		BMP_WriteFile(bmpFile, filename);
		BMP_Free(bmpFile);
	}
}

void GraphicIndex_Free(GraphicIndex* gfxRes) {
    for (uint32_t i = 0; i < gfxRes->numEntries; ++i) {
        if (gfxRes->entryData[i]) {
            Graphic_Free(gfxRes->entryData[i]);
        }
    }
    free(gfxRes->entryData);
    free(gfxRes);
}