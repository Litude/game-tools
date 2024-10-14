#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "qdbmp.h"
#include "palette.h"

const char MAGIC[24] = "RGE RLE shape file";


typedef struct
{
	char     version[4];
	int32_t  num_frames;
	char     comment[24];
} slpFileHeader;

typedef struct
{
	uint32_t cmd_table_offset;
	uint32_t outline_table_offset;
	uint32_t palette_offset;
	uint32_t properties;
	int32_t  width;
	int32_t  height;
	int32_t  hotspot_x;
	int32_t  hotspot_y;
} slpFrameInfo;

typedef struct
{
	int16_t  left;
    int16_t  right;
} slpRowEdge;

typedef struct 
{
	char     version[4];
	int32_t  num_frames;
} shpFileHeader;

typedef struct
{
    uint32_t data_offset;
    uint32_t palette_offset;
} shpFrameOffsets;

typedef struct
{
	int16_t bound_y;
	int16_t bound_x;
    int16_t origin_y;
    int16_t origin_x;
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
} shpFrameInfo;


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("No filename given");
        return EXIT_FAILURE;
    }
    FILE* inputFile = fopen(argv[1], "rb");
    if (!inputFile) {
        printf("File not found");
        return EXIT_FAILURE;
    }

    char* palettePath = argc >= 3 ? argv[2] : "PALETTE.PAL";

    uint8_t transparentColor = argc == 4 ? atoi(argv[3]) : 255;

    fseek(inputFile, 0, SEEK_END);
    size_t fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);
    
    printf("File size is %d\n", fileSize);

    uint8_t* fileData = malloc(fileSize);
    size_t readStuff = fread(fileData, 1, fileSize, inputFile);
    fclose(inputFile);
    printf("File read, got %d bytes\n", readStuff);

    uint8_t* palette = Palette_Create(palettePath);
    if (!palette) {
        free(fileData);
        printf("Error reading palette, terminating...\n");
        return EXIT_FAILURE;
    }
    

    char inputFileName[255];
    uint32_t size = strrchr(argv[1], '.') - argv[1];
    strncpy(inputFileName, argv[1], size);
    inputFileName[size] = '\0';
    char nameBuffer[255];

    if (!memcmp(fileData, "2.0N", 4)) {
        //SLP FILE
        printf("Detected SLP file format\n");
        slpFileHeader* header = (slpFileHeader*)fileData;

        if (memcmp(header->version, "2.0N", 4)) {
            printf("Invalid header!\n");
            Palette_Free(palette);
            free(fileData);
            return EXIT_FAILURE;
        }

        if (header->num_frames == 0) {
            printf("Bad frame count\n");
            Palette_Free(palette);
            free(fileData);
            return EXIT_FAILURE;
        }

        if (memcmp(MAGIC, header->comment, 24)) {
            printf("Bad magic!\n");
            Palette_Free(palette);
            free(fileData);
            return EXIT_FAILURE;
        }


        slpFrameInfo* frameData = (slpFrameInfo*)(fileData + sizeof(slpFileHeader));

        for (int32_t i = 0; i < header->num_frames; ++i) {
            slpFrameInfo* curFrame = &frameData[i];
            BMP* bmpFile = BMP_Create(curFrame->width, curFrame->height, 8);
            for (int32_t j = 0; j < 256; ++j) {
                BMP_SetPaletteColor(bmpFile, j, palette[j * 3], palette[j * 3 + 1], palette[j * 3 + 2]);
            }
            for (int32_t j = 0; j < curFrame->height; ++j) {
                for (int k = 0; k < curFrame->width; ++k) {
                    BMP_SetPixelIndex(bmpFile, k, j, transparentColor);
                }
            }
            for (int32_t y = 0; y < curFrame->height; ++y) {
                int32_t x = 0;
                slpRowEdge* edgeData = (slpRowEdge*)(fileData + curFrame->outline_table_offset + 4 * y);
                if (edgeData->left == 0x8000 || edgeData->right == 0x8000) continue;

                int16_t leftOffset = edgeData->left;
                int16_t rightOffset = edgeData->right; //Is this needed??

                uint32_t commandOffset = *(uint32_t*)(fileData + curFrame->cmd_table_offset + 4 * y);
                uint8_t* commandData = (fileData + commandOffset);

                bool rowEnded = false;

                while (!rowEnded) {
                    uint8_t commandByte = *commandData++;
                    uint8_t command = commandByte & 0xF;
                    switch (command) {
                        //Lesser draw
                        case 0x00:
                        case 0x04:
                        case 0x08:
                        case 0x0C: {
                            uint8_t length = commandByte >> 2;
                            for (int32_t i = 0; i < length; ++i) {
                                BMP_SetPixelIndex(bmpFile, x++ + leftOffset, y, *commandData++);
                            }
                            break;
                        }
                        //Lesser skip
                        case 0x01:
                        case 0x05:
                        case 0x09:
                        case 0x0D: {
                            uint8_t length = commandByte >> 2;
                            x += length;
                            break;
                        }
                        // Greater draw
                        case 0x02: {
                            uint32_t length = (((uint32_t)commandByte & 0xf0) << 4) + *commandData++;
                            for (uint32_t i = 0; i < length; ++i) {
                                BMP_SetPixelIndex(bmpFile, x++ + leftOffset, y, *commandData++);
                            }
                            break;
                        }
                        // Greater skip
                        case 0x03: {
                            uint32_t length = (((uint32_t)commandByte & 0xf0) << 4) + *commandData++;
                            x += length;
                            break;
                        }
                        // Player color
                        case 0x06: {
                            uint8_t length = commandByte >> 4; 
                            if (!length) {
                                length = *commandData++;
                            }
                            for (int32_t i = 0; i < length; ++i) {
                                uint8_t colorIndex = *commandData++;
                                uint8_t color = colorIndex + 16;
                                BMP_SetPixelIndex(bmpFile, x++ + leftOffset, y, color);
                            }
                            break;
                        }
                        // Fill
                        case 0x07: {
                            uint8_t length = commandByte >> 4; 
                            if (!length) {
                                length = *commandData++;
                            }
                            uint8_t color = *commandData++;
                            for (int32_t i = 0; i < length; ++i) {
                                BMP_SetPixelIndex(bmpFile, x++ + leftOffset, y, color);
                            }
                            break;
                        }
                        // Fill player color
                        case 0x0A: {
                            uint8_t length = commandByte >> 4; 
                            if (!length) {
                                length = *commandData++;
                            }
                            uint8_t colorIndex = *commandData++;
                            uint8_t color = colorIndex + 16;
                            for (int32_t i = 0; i < length; ++i) {
                                BMP_SetPixelIndex(bmpFile, x++ + leftOffset, y, color);
                            }
                            break;
                        }
                        // Fill shadow color
                        case 0x0B:{
                            uint8_t length = commandByte >> 4; 
                            if (!length) {
                                length = *commandData++;
                            }
                            uint8_t color = 0;
                            for (int32_t i = 0; i < length; ++i) {
                                BMP_SetPixelIndex(bmpFile, x++ + leftOffset, y, color);
                            }
                            break;
                        }
                        // End of row
                        case 0x0F:
                            rowEnded = true;
                            break;
                        default:
                            printf("Unimplemented command 0x%02x\n", command);
                            break;
                    }
                }
            }

            sprintf(nameBuffer, "%s_%d.bmp", inputFileName, i);

            printf("Writing file %s\n", nameBuffer);

            BMP_WriteFile(bmpFile, nameBuffer);
            BMP_Free(bmpFile);
            bmpFile = NULL;
        }
    }
    else if (!memcmp(fileData, "1.10", 4)) {
        //SHP File
        printf("Detected SHP file format\n");
        shpFileHeader* header = (shpFileHeader*)fileData;
        

        if (header->num_frames == 0) {
            printf("Bad frame count\n");
            Palette_Free(palette);
            free(fileData);
            return EXIT_FAILURE;
        }

        shpFrameOffsets* frameOffsets = (shpFrameOffsets*)(fileData + sizeof(shpFileHeader));

        for (int32_t i = 0; i < header->num_frames; ++i) {
            shpFrameInfo* curFrame = (shpFrameInfo*)(fileData + frameOffsets[i].data_offset);
            uint8_t* imageData = (fileData + frameOffsets[i].data_offset + sizeof(shpFrameInfo));

            int32_t height = curFrame->max_y - curFrame->min_y + 1;
            int32_t width = curFrame->max_x - curFrame->min_x + 1;
            
            BMP* bmpFile = BMP_Create(width, height, 8);
            for (int32_t j = 0; j < 256; ++j) {
                BMP_SetPaletteColor(bmpFile, j, palette[j * 3], palette[j * 3 + 1], palette[j * 3 + 2]);
            }
            for (int32_t j = 0; j < height; ++j) {
                for (int k = 0; k < width; ++k) {
                    BMP_SetPixelIndex(bmpFile, k, j, transparentColor);
                }
            }

            for (int32_t y = 0; y < height; ++y) {

                int32_t x = 0;
                bool rowEnded = false;
                while (!rowEnded) {
                    uint8_t command = *imageData++;
                    uint8_t count = command >> 1;
                    if (command == 1) {
                        //Skip
                        x += *imageData++;
                    } else if (command & 0x01) {
                        //Copy
                        for (int32_t j = 0; j < count; ++j) {
                            BMP_SetPixelIndex(bmpFile, x++, y, *imageData++);
                        }
                    } else if (command) {
                        //RLE
                        uint8_t color = *imageData++;
                        for (int32_t j = 0; j < count; ++j) {
                            BMP_SetPixelIndex(bmpFile, x++, y, color);
                        }
                    } else {
                        rowEnded = true;
                    }
                }
                
            }
            sprintf(nameBuffer, "%s_%d.bmp", inputFileName, i);

            printf("Writing file %s\n", nameBuffer);

            BMP_WriteFile(bmpFile, nameBuffer);
            BMP_Free(bmpFile);
            bmpFile = NULL;
        }
    } else {
        printf("Unknown file format\n");
    }

    Palette_Free(palette);
    free(fileData);
    return EXIT_SUCCESS;
}
