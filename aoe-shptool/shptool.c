#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

const char MAGIC[24] = "RGE RLE shape file";

typedef struct
{
	char     version[4];
	int32_t  num_frames;
	char     comment[24];
} SlpFileHeader;

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
} SlpFrameInfo;

typedef struct
{
	int16_t  left;
    int16_t  right;
} SlpRowEdge;

typedef struct 
{
	char     version[4];
	int32_t  num_frames;
} ShpFileHeader;

typedef struct
{
    uint32_t data_offset;
    uint32_t palette_offset;
} ShpFrameOffsets;

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
} ShpFrameInfo;


enum ShpCommandType {
    ShpCopy,
    ShpRle,
    ShpSkip,
    ShpEndOfRow
} ;

uint8_t END_OF_ROW_VALUE = 0;
uint8_t SKIP_VALUE = 1;
uint8_t RLE_PIXEL_COUNT = 3;

int32_t TRANSPARENT_ENTRY = -1;

int32_t boundX = -1;
int32_t boundY = -1;
int32_t originX = -1;
int32_t originY = -1;

bool hasColormap = false;
char colormapFilename[255];
int32_t colormap[256];

typedef struct {
    enum ShpCommandType commandType;
    uint8_t color;
    uint32_t count;
    int32_t* copyStart;
    FILE* file;
    uint32_t fileOffset;
} ShpWriteState;

typedef struct {
    int32_t* data;
    int32_t width;
    int32_t height;
} Buffer;

// Can we trust all SLPs to stay within the image bounds? These functions ensure no rules are broken
Buffer* Buffer_Create(int32_t width, int32_t height, int32_t defaultValue);
void Buffer_Free(Buffer* buffer);
int32_t Buffer_GetPixelData(Buffer* buffer, int32_t x, int32_t y);
void Buffer_SetPixelData(Buffer* buffer, int32_t x, int32_t y, int32_t value);

void writeCommand(ShpWriteState* shpWriteState);
void incrementCount(ShpWriteState* shpWriteState);

bool parseArguments(int argc, char* argv[]);
bool parseArgumentBounds(const char *arg, int *width, int *height);
bool parseArgumentOrigin(const char *arg, int *x, int *y);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

int main(int argc, char* argv[]) {
    if (!parseArguments(argc, argv)) {
        printf("Usage: shptool filename.slp [--bounds widthxheight] [--origin x,y] [--colormap colormap.col]");
        return EXIT_FAILURE;
    }

    if (hasColormap) {
        FILE* colormapFile = fopen(colormapFilename, "r");
        if (!colormapFile) {
            printf("Colormap file %s not found\n", colormapFilename);
            return EXIT_FAILURE;
        }
        // Default initialize colormap;
        for (int32_t i = 0; i < 256; ++i) {
            colormap[i] = i;
        }

        int32_t colormapLine = 0;

        while (colormapLine < 256 && fscanf(colormapFile, "%d", &colormap[colormapLine]) == 1) {
            if (colormap[colormapLine] < 0 || colormap[colormapLine] > 255) {
                printf("Error: Colormap number out of range (0-255) on line %d\n", colormapLine + 1);
                fclose(colormapFile);
                return EXIT_FAILURE;
            }
            colormapLine++;
        }
        fclose(colormapFile);

        if (colormapLine < 256) {
            printf("Colormap file %s was not a valid colormap, only read %d lines successfully\n", colormapFilename, colormapLine);
            return EXIT_FAILURE;
        }
        printf("Colormap read successfully\n");
    }

    FILE* inputFile = fopen(argv[1], "rb");
    if (!inputFile) {
        printf("Input file %s not found\n", argv[1]);
        return EXIT_FAILURE;
    }

    fseek(inputFile, 0, SEEK_END);
    size_t fileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);
    
    printf("Input file size is %d\n", fileSize);

    uint8_t* fileData = malloc(fileSize);
    size_t readStuff = fread(fileData, 1, fileSize, inputFile);
    fclose(inputFile);
    printf("Input file read, got %d bytes\n", readStuff);
    

    char inputFileName[255];
    uint32_t size = strrchr(argv[1], '.') - argv[1];
    strncpy(inputFileName, argv[1], size);
    inputFileName[size] = '\0';
    char nameBuffer[255];

    if (!memcmp(fileData, "2.0N", 4)) {
        //SLP FILE
        printf("Detected SLP file format\n");
        SlpFileHeader* slpHeader = (SlpFileHeader*)fileData;

        if (memcmp(slpHeader->version, "2.0N", 4)) {
            printf("Invalid header!\n");
            free(fileData);
            return EXIT_FAILURE;
        }

        if (slpHeader->num_frames == 0) {
            printf("Bad frame count\n");
            free(fileData);
            return EXIT_FAILURE;
        }

        if (memcmp(MAGIC, slpHeader->comment, 24)) {
            printf("Bad magic!\n");
            free(fileData);
            return EXIT_FAILURE;
        }


        SlpFrameInfo* slpFrameData = (SlpFrameInfo*)(fileData + sizeof(SlpFileHeader));

        int32_t frameCount = slpHeader->num_frames;

        Buffer** frameBuffers = calloc(frameCount, sizeof(*frameBuffers));


        for (int32_t i = 0; i < frameCount; ++i) {
            //printf("Reading SLP frame %d\n", i + 1);
            SlpFrameInfo* curFrame = &slpFrameData[i];
            frameBuffers[i] = Buffer_Create(curFrame->width, curFrame->height, TRANSPARENT_ENTRY);
            Buffer* frameBuffer = frameBuffers[i];

            for (int32_t y = 0; y < curFrame->height; ++y) {
                int32_t x = 0;
                SlpRowEdge* edgeData = (SlpRowEdge*)(fileData + curFrame->outline_table_offset + 4 * y);
                if (edgeData->left == INT16_MIN || edgeData->right == INT16_MIN) continue;

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
                                Buffer_SetPixelData(frameBuffer, x++ + leftOffset, y, *commandData++);
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
                                Buffer_SetPixelData(frameBuffer, x++ + leftOffset, y, *commandData++);
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
                                Buffer_SetPixelData(frameBuffer, x++ + leftOffset, y, color);
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
                                Buffer_SetPixelData(frameBuffer, x++ + leftOffset, y, color);
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
                                Buffer_SetPixelData(frameBuffer, x++ + leftOffset, y, color);
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
                                Buffer_SetPixelData(frameBuffer, x++ + leftOffset, y, color);
                            }
                            break;
                        }
                        // Extended command
                        case 0x0E: {
                            uint8_t extendedData = *commandData++;
                            printf("Extended command 0x%02x\n", extendedData);
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

        }

        printf("Finished reading %s.slp %d SLP frames\n", inputFileName, frameCount);

        
        ShpFrameOffsets** frameOffsets = calloc(frameCount, sizeof(*frameOffsets));

        sprintf(nameBuffer, "%s.shp", inputFileName);
        FILE* outputFile = fopen(nameBuffer, "wb");
        fwrite("1.10", 1, 4, outputFile);
        fwrite(&frameCount, 4, 1, outputFile);

        ShpFrameOffsets dummyOffsets = {
            .data_offset = 0,
            .palette_offset = 0
        };

        // Write temporary blank offsets, this will be filled in later...
        for (int32_t i = 0; i < frameCount; ++i) {
            frameOffsets[i] = calloc(1, sizeof(ShpFrameOffsets));
            fwrite(&dummyOffsets.data_offset, 4, 1, outputFile);
            fwrite(&dummyOffsets.palette_offset, 4, 1, outputFile);
        }

        int32_t currentOffset = frameCount * 8 + 8; // one frame has 8 bytes of offsets and the header is 8 bytes (1.10 + frame count)

        // Bounds is the size of the original FLC image (not stored in SLPs, but usually 640x480, 800x600 or 1024x768)
        // Calculate the largest width and height of a frame and round up
        // Origin is the same as hotspot but in the coordinate system of the original FLC image. This is also not recoverable from an SLP.
        // Have to assume that the origin is the center if the image still fits or otherwise 0,0.

        // Auto detect bounds
        if (boundX < 0 || boundY < 0) {
            for (int32_t i = 0; i < frameCount; ++i) {
                SlpFrameInfo* slpFrameInfo = &slpFrameData[i];
                int32_t frameWidth = slpFrameInfo->width;
                if (frameWidth <= 640) {
                    boundX = MAX(boundX, 640);
                    boundY = MAX(boundY, 480);
                }
                else if (frameWidth <= 800) {
                    boundX = MAX(boundX, 800);
                    boundY = MAX(boundY, 600);
                }
                else if (frameWidth <= 1024) {
                    boundX = MAX(boundX, 1024);
                    boundY = MAX(boundY, 768);
                }
                else {
                    boundX = MAX(boundX, frameWidth);
                }

                int32_t frameHeight  =slpFrameInfo->height;
                if (frameHeight <= 480) {
                    boundX = MAX(boundX, 640);
                    boundY = MAX(boundY, 480);
                }
                else if (frameHeight <= 600) {
                    boundX = MAX(boundX, 800);
                    boundY = MAX(boundY, 600);
                }
                else if (frameHeight <= 768) {
                    boundX = MAX(boundX, 1024);
                    boundY = MAX(boundY, 768);
                }
                else {
                    boundY = MAX(boundY, frameHeight);
                }
            }
        }

        // Autodetect origin
        // If the hotspot is zero, we assume the origin was also zero.
        // Else we assume that the origin was the middle of the image (if the image fits! Else we set it to zero)
        if (originX < 0 || originY < 0) {
            bool allHotspotsZero = true;
            for (int32_t i = 0; i < frameCount; ++i) {
                SlpFrameInfo* slpFrameInfo = &slpFrameData[i];
                if (slpFrameInfo[i].hotspot_x != 0 || slpFrameInfo[i].hotspot_y != 0) {
                    allHotspotsZero = false;
                    break;
                }
            }
            if (allHotspotsZero) {
                originX = 0;
                originY = 0;
            }
            else {
                // Check if setting the origin to the middle of the image allows the image to fit within the bounds
                int32_t minX = INT32_MAX;
                int32_t minY = INT32_MAX;
                int32_t maxX = INT32_MIN;
                int32_t maxY = INT32_MIN;
                for (int32_t i = 0; i < frameCount; ++i) {
                    SlpFrameInfo* slpFrameInfo = &slpFrameData[i];
                    minX = MIN(minX, -slpFrameInfo->hotspot_x);
                    minY = MIN(minX, -slpFrameInfo->hotspot_y);
                    maxX = MAX(maxX, slpFrameInfo->width - slpFrameInfo->hotspot_x - 1);
                    maxY = MAX(maxY, slpFrameInfo->height - slpFrameInfo->hotspot_y - 1);
                }

                if (boundX / 2 - minX >= 0 && boundY / 2 - minY >= 0 && boundX / 2 + maxX < boundX && boundY / 2 + maxY < boundY) {
                    originX = boundX / 2;
                    originY = boundY / 2;
                }
                else {
                    originX = 0;
                    originY = 0;
                }
            }
        }

        if (hasColormap) {
            for (int32_t i = 0; i < frameCount; ++i) {
                SlpFrameInfo* slpFrameInfo = &slpFrameData[i];
                for (int32_t y = 0; y < slpFrameInfo->height; ++y) {
                    for (int32_t x = 0; x < slpFrameInfo->width; ++x) {
                        int32_t currentValue = Buffer_GetPixelData(frameBuffers[i], x, y);
                        if (currentValue != TRANSPARENT_ENTRY) {
                            Buffer_SetPixelData(frameBuffers[i], x, y, colormap[currentValue]);
                        }
                    }
                }
            }
        }

        for (int32_t i = 0; i < frameCount; ++i) {
            //printf("Writing SHP frame %d\n", i + 1);
            SlpFrameInfo* slpFrameInfo = &slpFrameData[i];
            frameOffsets[i]->data_offset = currentOffset;

            ShpFrameInfo shpFrameInfo = {
                .bound_y = boundY,
                .bound_x = boundX,
                .origin_y = originY,
                .origin_x = originX,
                .min_x = -slpFrameInfo->hotspot_x,
                .min_y = -slpFrameInfo->hotspot_y,
                .max_x = slpFrameInfo->width - slpFrameInfo->hotspot_x - 1,
                .max_y = slpFrameInfo->height - slpFrameInfo->hotspot_y - 1,
            };

            fwrite(&shpFrameInfo.bound_y, sizeof(shpFrameInfo.bound_y), 1, outputFile);
            fwrite(&shpFrameInfo.bound_x, sizeof(shpFrameInfo.bound_x), 1, outputFile);
            fwrite(&shpFrameInfo.origin_y, sizeof(shpFrameInfo.origin_y), 1, outputFile);
            fwrite(&shpFrameInfo.origin_x, sizeof(shpFrameInfo.origin_x), 1, outputFile);
            fwrite(&shpFrameInfo.min_x, sizeof(shpFrameInfo.min_x), 1, outputFile);
            fwrite(&shpFrameInfo.min_y, sizeof(shpFrameInfo.min_y), 1, outputFile);
            fwrite(&shpFrameInfo.max_x, sizeof(shpFrameInfo.max_x), 1, outputFile);
            fwrite(&shpFrameInfo.max_y, sizeof(shpFrameInfo.max_y), 1, outputFile);
            currentOffset += 4 * sizeof(int16_t) + 4 * sizeof(int32_t);

            int32_t* frameBuffer = frameBuffers[i]->data;

            ShpWriteState shpWriteState = {
                .commandType = ShpEndOfRow,
                .color = 0,
                .count = 0,
                .copyStart = NULL,
                .file = outputFile,
                .fileOffset = currentOffset
            };

            // uint8_t previousValue = 0;

            // enum ShpCommandType currentCommand = ShpEndOfRow;
            // uint8_t currentCount = 0;

            for (int32_t y = 0; y < slpFrameInfo->height; ++y) {
                for (int32_t x = 0; x < slpFrameInfo->width; ++x) {
                    int32_t rawValue = frameBuffer[y * slpFrameInfo->width + x];
                    if (rawValue == TRANSPARENT_ENTRY) {
                        if (shpWriteState.commandType == ShpSkip) {
                            incrementCount(&shpWriteState);
                        }
                        else {
                            //printf("Setting mode Skip\n");
                            writeCommand(&shpWriteState);
                            // write current command
                            shpWriteState.commandType = ShpSkip;
                            shpWriteState.count = 1;
                        }
                    }
                    else {
                        // We should only have actual color values and transparent
                        if (rawValue < 0 || rawValue > 255) {
                            printf("Color value is out of bounds, got %d!\n", rawValue);
                        }
                        uint8_t currentValue = (uint8_t)rawValue;
                        if (shpWriteState.commandType == ShpRle && currentValue == shpWriteState.color) {
                            incrementCount(&shpWriteState);
                        }
                        else {
                            bool useRle = false;
                            // This is needed to match original SHP files
                            int32_t rlePixelCount = shpWriteState.commandType == ShpCopy ? 3 : 2;
                            if (x + (rlePixelCount - 1) < slpFrameInfo->width)  {
                                useRle = true;
                                for (int32_t i = 1; i < rlePixelCount && useRle; ++i) {
                                    useRle = useRle && (frameBuffer[y * slpFrameInfo->width + x + i] == currentValue);
                                }
                            }
                            if (useRle) {
                                writeCommand(&shpWriteState);
                                shpWriteState.commandType = ShpRle;
                                shpWriteState.color = currentValue;
                                shpWriteState.count = 1;
                            }
                            else if (shpWriteState.commandType == ShpCopy) {
                                incrementCount(&shpWriteState);
                            }
                            else {
                                writeCommand(&shpWriteState);
                                shpWriteState.commandType = ShpCopy;
                                shpWriteState.copyStart = &frameBuffer[y * slpFrameInfo->width + x];
                                shpWriteState.count = 1;
                            }

                        }
                    }
                }

                // Writing a skip is pointless if the next command is end of row, so skip writing it
                if (shpWriteState.commandType != ShpSkip) {
                    writeCommand(&shpWriteState);
                }
                
                fwrite(&END_OF_ROW_VALUE, 1, 1, shpWriteState.file);
                shpWriteState.fileOffset += 1;
                shpWriteState.color = 0;
                shpWriteState.commandType = ShpEndOfRow;
                shpWriteState.copyStart = NULL;
                shpWriteState.count = 0;

            }
            currentOffset = shpWriteState.fileOffset;
        }

        for (int32_t i = 0; i < frameCount; ++i) {
            fseek(outputFile, 8 + i * 8, SEEK_SET);
            fwrite(&frameOffsets[i]->data_offset, 4, 1, outputFile);
        }


        for (int32_t i = 0; i < frameCount; ++i) {
            free(frameOffsets[i]);
            Buffer_Free(frameBuffers[i]);
        }
        free(frameOffsets);
        free(frameBuffers);

        fclose(outputFile);
        printf("Finished writing %s.shp %d SHP frames\n", inputFileName, frameCount);


    } else {
        printf("Unknown file format\n");
    }

    free(fileData);
    return EXIT_SUCCESS;
}


void incrementCount(ShpWriteState* shpWriteState) {
    shpWriteState->count += 1;
    // To provide the most efficient implementation, we could write these two commands directly so the following
    // data knows the real state of the command data.
    // HOWEVER it seems official SHPs did not do this, and we want to match them even in the encoding method as
    // closely as possible
    // if (shpWriteState->count == 127) {
    //     if (shpWriteState->commandType == ShpCopy || shpWriteState->commandType == ShpRle) {
    //         writeCommand(shpWriteState);
    //     }
    // }
}

void writeCommand(ShpWriteState* shpWriteState) {

    uint8_t maxValue = shpWriteState->commandType == ShpSkip ? 255 : 127; 
    
    uint8_t currentWriteCount = MIN(shpWriteState->count, maxValue);
    while (currentWriteCount > 0) {
        switch (shpWriteState->commandType) {
            // End of row is used as "no-command" selected so it is written separately
            case ShpRle: {
                uint8_t commandValue = currentWriteCount << 1;
                fwrite(&commandValue, 1, 1, shpWriteState->file);
                fwrite(&shpWriteState->color, 1, 1, shpWriteState->file);
                shpWriteState->fileOffset += 2;
                break;
            }
            case ShpCopy: {
                uint8_t commandValue = (currentWriteCount << 1) | 1;
                fwrite(&commandValue, 1, 1, shpWriteState->file);
                for (int32_t i = 0; i < currentWriteCount; ++i) {
                    uint8_t currentValue = (uint8_t)shpWriteState->copyStart[i];
                    fwrite(&currentValue, 1, 1, shpWriteState->file);
                }
                shpWriteState->fileOffset += 1 + currentWriteCount;
                shpWriteState->copyStart += currentWriteCount;
                break;
            }
            case ShpSkip: {
                fwrite(&SKIP_VALUE, 1, 1, shpWriteState->file);
                fwrite(&currentWriteCount, 1, 1, shpWriteState->file);
                shpWriteState->fileOffset += 2;
                break;
            }
            
            default:
                break;
        }
        shpWriteState->count -= currentWriteCount;
        currentWriteCount = currentWriteCount = MIN(shpWriteState->count, maxValue);
    }

    shpWriteState->commandType = ShpEndOfRow;
    shpWriteState->count = 0;
    shpWriteState->color = 0;
}

bool parseArguments(int argc, char* argv[]) {
    if (argc < 2 || strstr(argv[1], "--") == argv[1]) {
        return false;
    }
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--bounds") == 0) {
            if (i + 1 < argc && parseArgumentBounds(argv[i + 1], &boundX, &boundY)) {
                i++;
            } else {
                fprintf(stderr, "Error: Invalid --bounds argument\n");
                return false;
            }
        } else if (strcmp(argv[i], "--origin") == 0) {
            if (i + 1 < argc && parseArgumentOrigin(argv[i + 1], &originX, &originY)) {
                i++;
            } else {
                fprintf(stderr, "Error: Invalid --origin argument\n");
                return false;
            }
        } else if (strcmp(argv[i], "--colormap") == 0) {
            if (i + 1 < argc) {
                hasColormap = true;
                strncpy(colormapFilename, argv[i + 1], sizeof(colormapFilename) - 1);
                colormapFilename[sizeof(colormapFilename) - 1] = '\0';
                i++;
            } else {
                fprintf(stderr, "Error: Missing filename for --colormap\n");
                return false;
            }
        } else {
            fprintf(stderr, "Error: Unknown argument '%s'\n", argv[i]);
            return false;
        }
    }
    return true;
}

bool parseArgumentBounds(const char *arg, int *width, int *height) {
    return sscanf(arg, "%dx%d", width, height) == 2;
}

bool parseArgumentOrigin(const char *arg, int *x, int *y) {
    return sscanf(arg, "%d,%d", x, y) == 2;
}

Buffer* Buffer_Create(int32_t width, int32_t height, int32_t defaultValue) {
    Buffer* result = calloc(1, sizeof(Buffer));
    result->height = height;
    result->width = width;
    result->data = malloc(width * height * sizeof(*result->data));
    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            result->data[y * width + x] = defaultValue;
        }
    }
    return result;
}

void Buffer_Free(Buffer* buffer) {
    free(buffer->data);
    free(buffer);
}

int32_t Buffer_GetPixelData(Buffer* buffer, int32_t x, int32_t y) {

    if (x >= 0 && x < buffer->width && y >= 0 && y < buffer->height) {
        return buffer->data[y * buffer->width + x];
    }
    else {
        printf("Received out of bounds get for buffer pixel data!\n");
        return TRANSPARENT_ENTRY;
    }
}

void Buffer_SetPixelData(Buffer* buffer, int32_t x, int32_t y, int32_t value) {

    if (x >= 0 && x < buffer->width && y >= 0 && y < buffer->height) {
        buffer->data[y * buffer->width + x] = value;
    }
    else {
        printf("Ignoring out of bounds buffer write to %dx%d (buffer size is %dx%d)!\n", x, y, buffer->width, buffer->height);
    }
}
