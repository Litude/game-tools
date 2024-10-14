#include <stdint.h>
#include <stdio.h>

#define MUS_SIGNATURE "MUS\x1A"
#define MUS_SIGNATURE_LENGTH 4

enum
{
	EVENT_NOTE_RELEASE,
	EVENT_NOTE_PLAY,
	EVENT_PITCH_BEND,
    EVENT_SYSTEM,
	EVENT_CONTROLLER,
	EVENT_END_OF_MEASURE,
	EVENT_END_OF_DATA
};

typedef struct {
    char sig[4];
    uint16_t lenSong;
    uint16_t offSong;
    uint16_t primaryChannels;
    uint16_t secondaryChannels;
    uint16_t numInstruments;
    uint16_t reserved;
    uint16_t* instruments;
} musHeader;

typedef struct {
    int8_t note;
} releaseNoteData;

typedef struct {
    int8_t note;
    int8_t volume;
} playNoteData;

typedef struct {
    uint8_t amount;
} pitchBendData;

typedef struct {
    int8_t controller;
} systemEventData;

typedef struct {
    int8_t controller;
    int8_t value;
} controllerEventData;

typedef union {
    releaseNoteData releaseNote;
    playNoteData playNote;
    pitchBendData pitchBend;
    systemEventData systemEvent;
    controllerEventData controllerEvent;
} eventData;

typedef struct {
    uint8_t channel;
    uint8_t type;
    eventData event;
} musEvent;

typedef struct {
    uint16_t numEvents;
    musEvent* events;
    uint32_t delay;
} musTick;

typedef struct {
    musHeader header;
    uint16_t numTicks;
    musTick* ticks;
} musFile;

musFile* createMusFromStream(FILE* stream);
bool writeMusToStream(musFile* file, FILE* stream);
void freeMusFile(musFile* fileEntry);
void doubleMusDelays(musFile* fileEntry);
