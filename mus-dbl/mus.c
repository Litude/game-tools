#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "mus.h"

musFile* createMusFromStream(FILE* stream) {
    musFile* entry = malloc(sizeof(musFile));
    if (!entry) {
        printf("malloc failed\n");
        freeMusFile(entry);
        return NULL;
    }

    entry->numTicks = 0;
    fread(&entry->header.sig, sizeof(char), MUS_SIGNATURE_LENGTH, stream);

    if (memcmp(&entry->header.sig, MUS_SIGNATURE, MUS_SIGNATURE_LENGTH)) {
        printf("File does not have a valid MUS signature, skipping...\n");
        freeMusFile(entry);
        return NULL;
    }

    fread(&entry->header.lenSong, sizeof(uint16_t), 1, stream);
    fread(&entry->header.offSong, sizeof(uint16_t), 1, stream);
    fread(&entry->header.primaryChannels, sizeof(uint16_t), 1, stream);
    fread(&entry->header.secondaryChannels, sizeof(uint16_t), 1, stream);
    fread(&entry->header.numInstruments, sizeof(uint16_t), 1, stream);
    fread(&entry->header.reserved, sizeof(uint16_t), 1, stream);

    entry->header.instruments = malloc(entry->header.numInstruments * sizeof(uint16_t));
    if (!entry->header.instruments) {
        printf("malloc failed\n");
        freeMusFile(entry);
        return NULL;
    }
    for (uint32_t i = 0; i < entry->header.numInstruments; ++i) {
        fread(&entry->header.instruments[i], sizeof(uint16_t), 1, stream);
    }

    if (ftell(stream) != (long)entry->header.offSong) {
        printf("Padding in header not supported\n");
        freeMusFile(entry);
        return NULL;
    }

    uint8_t buffer1;
    uint8_t buffer2;
    uint8_t buffer3;

    entry->ticks = calloc(16, sizeof(musTick));
    if (!entry->ticks) {
        printf("malloc failed\n");
        freeMusFile(entry);
        return NULL;
    }

    while (fread(&buffer1, sizeof(uint8_t), 1, stream)) {
        uint8_t event = buffer1 >> 4 & 0x07;
        uint8_t channel = buffer1 & 0x0F;
        bool last = buffer1 >> 7;

        musTick* currentTick = &entry->ticks[entry->numTicks];

        if (currentTick->numEvents % 16 == 0) {
            currentTick->events = realloc(currentTick->events, (currentTick->numEvents + 16) * sizeof(musEvent));
            if (!currentTick->events) {
                printf("malloc failed\n");
                freeMusFile(entry);
            }
            memset(&currentTick->events[currentTick->numEvents], 0, 16 * sizeof(musEvent));
        }

        musEvent* currentEvent = &currentTick->events[currentTick->numEvents];

        currentEvent->type = event;
        currentEvent->channel = channel;

        switch (event) {
            case EVENT_NOTE_RELEASE:
                fread(&buffer2, sizeof(uint8_t), 1, stream);
                currentEvent->event.releaseNote.note = (int8_t)buffer2;
                break;

            case EVENT_NOTE_PLAY:
                fread(&buffer2, sizeof(uint8_t), 1, stream);
                currentEvent->event.playNote.note = buffer2 & ~0x80;
                if (buffer2 & 0x80) {
                    fread(&buffer3, sizeof(uint8_t), 1, stream);
                    currentEvent->event.playNote.volume = (int8_t)buffer3;
                } else {
                    currentEvent->event.playNote.volume = -1;
                }
                break;

            case EVENT_PITCH_BEND:
                fread(&buffer2, sizeof(uint8_t), 1, stream);
                currentEvent->event.pitchBend.amount = buffer2;
                break;

            case EVENT_SYSTEM:
                fread(&buffer2, sizeof(uint8_t), 1, stream);
                currentEvent->event.systemEvent.controller = (int8_t)buffer2;
                break;

            case EVENT_CONTROLLER:
                fread(&buffer2, sizeof(uint8_t), 1, stream);
                currentEvent->event.controllerEvent.controller = (int8_t)buffer2;
                fread(&buffer3, sizeof(uint8_t), 1, stream);
                currentEvent->event.controllerEvent.value = (int8_t)buffer3;
                break;

            case EVENT_END_OF_MEASURE:
                //Event only has type, no need for further processing
                break;

            case EVENT_END_OF_DATA:
                ++currentTick->numEvents;
                ++entry->numTicks;
                return entry;

            default:
                printf("Encountered unknown event %d, aborting\n", event);
                freeMusFile(entry);
                return NULL;
        }

        ++currentTick->numEvents;

        if (last) {
            uint8_t delayBuffer = 0;
            uint32_t delay = 0;
            do {
                fread(&delayBuffer, sizeof(uint8_t), 1, stream);
                delay <<= 7;
                delay += delayBuffer & ~0x80;
            } while (delayBuffer & 0x80);
            currentTick->delay = delay;

            ++entry->numTicks;
            if (entry->numTicks % 16 == 0) {
                entry->ticks = realloc(entry->ticks, (entry->numTicks + 16) * sizeof(musTick));
                if (!entry->ticks) {
                    printf("malloc failed\n");
                    freeMusFile(entry);
                }
                memset(&entry->ticks[entry->numTicks], 0, 16 * sizeof(musTick));
            }
        }
    }
    printf("Unexpected end of data\n");
    freeMusFile(entry);
    return NULL;

}

bool writeMusToStream(musFile* file, FILE* stream) {
    fwrite(&file->header.sig, sizeof(char), MUS_SIGNATURE_LENGTH, stream);
    fwrite(&file->header.lenSong, sizeof(uint16_t), 1, stream);
    fwrite(&file->header.offSong, sizeof(uint16_t), 1, stream);
    fwrite(&file->header.primaryChannels, sizeof(uint16_t), 1, stream);
    fwrite(&file->header.secondaryChannels, sizeof(uint16_t), 1, stream);
    fwrite(&file->header.numInstruments, sizeof(uint16_t), 1, stream);
    fwrite(&file->header.reserved, sizeof(uint16_t), 1, stream);

    for (uint32_t i = 0; i < file->header.numInstruments; ++i) {
        fwrite(&file->header.instruments[i], sizeof(uint16_t), 1, stream);
    }

    for (uint32_t i = 0; i < file->numTicks; ++i) {
        musTick* currentTick = &file->ticks[i];

        for (uint32_t j = 0; j < currentTick->numEvents; ++j) {
            musEvent* currentEvent = &currentTick->events[j];

            uint8_t event = currentEvent->type;
            uint8_t channel = currentEvent->channel;
            bool isLast = j == currentTick->numEvents - 1 && event != EVENT_END_OF_DATA;

            uint8_t mainByte = isLast << 7 | event << 4 | channel;
            fwrite(&mainByte, sizeof(uint8_t), 1, stream);

            switch (event) {
                case EVENT_NOTE_RELEASE:
                    fwrite(&currentEvent->event.releaseNote.note, sizeof(int8_t), 1, stream);
                    break;

                case EVENT_NOTE_PLAY: {
                    uint8_t eventByte = (uint8_t)currentEvent->event.playNote.note;
                    if (currentEvent->event.playNote.volume != -1) {
                        eventByte |= 0x80;
                        fwrite(&eventByte, sizeof(uint8_t), 1, stream);
                        fwrite(&currentEvent->event.playNote.volume, sizeof(int8_t), 1, stream);
                    } else {
                        fwrite(&eventByte, sizeof(uint8_t), 1, stream);
                    }
                    break;
                }

                case EVENT_PITCH_BEND:
                    fwrite(&currentEvent->event.pitchBend.amount, sizeof(uint8_t), 1, stream);
                    break;

                case EVENT_SYSTEM:
                    fwrite(&currentEvent->event.systemEvent.controller, sizeof(int8_t), 1, stream);
                    break;

                case EVENT_CONTROLLER:
                    fwrite(&currentEvent->event.controllerEvent.controller, sizeof(int8_t), 1, stream);
                    fwrite(&currentEvent->event.controllerEvent.value, sizeof(int8_t), 1, stream);
                    break;

                case EVENT_END_OF_MEASURE:
                    //Event only has type, no need for further processing
                    break;

                case EVENT_END_OF_DATA:
                    return true;

                default:
                    printf("Encountered unknown event %d, aborting\n", event);
                    return false;
            }

            if (isLast) {
                uint32_t delay = currentTick->delay;
                uint8_t delayBytes[5] = { 0, 0, 0, 0, 0 };

                uint32_t k;
                for (k = 0; k < 5 && delay > 0; ++k) {
                    delayBytes[k] = delay & 0x7F;
                    delay >>= 7;
                    if (k > 0) {
                        delayBytes[k] |= 0x80;
                    }
                }

                for (int32_t l = k - 1; l >= 0; --l) {
                    fwrite(&delayBytes[l], sizeof(uint8_t), 1, stream);
                }

            }

        }
    }
}

void freeMusFile(musFile* fileEntry) {
    if (fileEntry) {
        free(fileEntry->header.instruments);
        if (fileEntry->ticks) {
            for (uint32_t i = 0; i < fileEntry->numTicks; ++i) {
                free(fileEntry->ticks[i].events);
            }
            free(fileEntry->ticks);
        }
        free(fileEntry);
    }
}

void doubleMusDelays(musFile* fileEntry) {
    if (fileEntry) {
        for (uint32_t i = 0; i < fileEntry->numTicks; ++i) {
            fileEntry->ticks[i].delay *= 2;
        }
    }
}
