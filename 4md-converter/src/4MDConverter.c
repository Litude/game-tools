#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
	#include <limits.h>
	#include <Winsock2.h>
	#define PATH_MAX MAX_PATH
#else
	#include <netinet/in.h>
	#include <linux/limits.h>
#endif

#define PATTERN_LENGTH 64
#define NUM_CHANNELS 4
#define MAX_SAMPLES 31

#pragma pack(push, 1)
/* Sample entry in MOD header */
typedef struct {
	char name[22];
	unsigned short length;
	unsigned char finetune;
	unsigned char volume;
	unsigned short repeat_start;
	unsigned short repeat_length;
} SAMPLEENTRY;

/* MOD file header structure */
typedef struct {
	char name[20];
	SAMPLEENTRY sample_entry[31];
	unsigned char num_rows;
	unsigned char max_rows;
	unsigned char row_pattern[128];
	char sig[4];
} MODHEADER;

/* Expanded form of a pattern note */
typedef struct {
	unsigned char sample;
	unsigned short period;
	unsigned short effect;
} NOTE;

/* Structure for holding an entire MOD file */
typedef struct {
	MODHEADER header;
	NOTE ***note;
	signed char **sample;
	int num_patterns;
} MODFILE;
#pragma pack(pop)

typedef enum {
	SAMPLE_TYPE_UNSPECIFIED = 0,
	SAMPLE_TYPE_4BIT,
	SAMPLE_TYPE_8BIT
} SAMPLETYPE;

typedef struct {
	SAMPLETYPE input_sample;
	SAMPLETYPE output_sample;
	unsigned char flags;
} SETTINGS;

enum SettingsFlag {
	FLAG_CLEAN_ROW_PATTERNS = 0x01
};

const signed char positiveAmplitudes[] = {0, 18, 36, 54, 72, 90, 108, 127};

/* Scales the signed 4 bit value to a signed 8 bit value */
signed char get_scaled_amplitude(signed char original_amplitude) {
	if (original_amplitude & 0x08) {
		//for negative 4 bit values, a simple left shift is enough
		return original_amplitude << 4;
	} else {
		return positiveAmplitudes[original_amplitude];
	}
}

/* Reads a single pattern note and expands it */
void read_note(FILE *input_file, NOTE *note)
{
	unsigned char byte_array[4];

	fread(&byte_array, 1, 4, input_file);
	note->sample = (byte_array[0] & 0xF0) | ((byte_array[2] & 0xF0) >> 4);
	note->period = ((byte_array[0] & 0x0F) << 8) | byte_array[1];
	note->effect = ((byte_array[2] & 0x0F) << 8) | byte_array[3];
}

/* Compacts and writes a single pattern note */
void write_note(FILE *output_file, NOTE *note)
{
	unsigned char byte_array[4];

	byte_array[0] = (note->sample & 0xF0) | (note->period >> 8);
	byte_array[1] = (note->period & 0x00FF);
	byte_array[2] = ((note->sample & 0x0F) << 4) | (note->effect >> 8);
	byte_array[3] = (note->effect & 0x00FF);
	fwrite(&byte_array, 1, 4, output_file);
}

void read_mod_file(FILE *input_file, MODFILE *mod_file, SAMPLETYPE sample_type)
{
	int i, j, k;

	/* Read in MOD header */
	fread(&(mod_file->header), sizeof(MODHEADER), 1, input_file);

	/* Need to fix endian order (MOD & 4MD use big endian) */
	for (i = 0; i < 31; i++)
	{
		mod_file->header.sample_entry[i].length = ntohs(mod_file->header.sample_entry[i].length);
		mod_file->header.sample_entry[i].repeat_start = ntohs(mod_file->header.sample_entry[i].repeat_start);
		mod_file->header.sample_entry[i].repeat_length = ntohs(mod_file->header.sample_entry[i].repeat_length);
	}

	/* Find out number of patterns */
	mod_file->num_patterns = 1;
	for (i = 0; i < mod_file->header.num_rows; i++)
	{
		if (mod_file->header.row_pattern[i] + 1 > mod_file->num_patterns)
			mod_file->num_patterns = mod_file->header.row_pattern[i] + 1;
	}

	/* Allocate buffers for expanded patterns */
	mod_file->note = malloc(mod_file->num_patterns * sizeof(NOTE **));
	for (i = 0; i < mod_file->num_patterns; i++)
	{
		mod_file->note[i] = malloc(NUM_CHANNELS * sizeof(NOTE *));
		for (j = 0; j < NUM_CHANNELS; j++)
			mod_file->note[i][j] = malloc(PATTERN_LENGTH * sizeof(NOTE));
	}

	/* Read in pattern data */
	for (i = 0; i < mod_file->num_patterns; i++)
		for (k = 0; k < PATTERN_LENGTH; k++)
			for (j = 0; j < NUM_CHANNELS; j++)
				read_note(input_file, &(mod_file->note[i][j][k]));

	/* Allocate buffers for samples */
	mod_file->sample = malloc(MAX_SAMPLES * sizeof(unsigned char *));

	/* Read in samples */
	for (i = 0; i < MAX_SAMPLES; i++)
	{
		if (mod_file->header.sample_entry[i].length > 0)
		{
			mod_file->sample[i] = malloc(mod_file->header.sample_entry[i].length * 2);
			memset(mod_file->sample[i], 0, mod_file->header.sample_entry[i].length * 2);

			if (sample_type == SAMPLE_TYPE_4BIT) {
				for (j = 0; j < mod_file->header.sample_entry[i].length; j++) {
					signed char first = 0;
					signed char second = 0;
					unsigned char buffer = 0; //unsigned so that bit shift fills zeros
					fread(&buffer, 1, 1, input_file);
					first = get_scaled_amplitude((buffer & 0xF0) >> 4);
					second = get_scaled_amplitude(buffer & 0x0F);

					mod_file->sample[i][j * 2] = first;
					mod_file->sample[i][j * 2 + 1] = second;
				}
			} else {
				fread(mod_file->sample[i], 1, mod_file->header.sample_entry[i].length * 2, input_file);
			}

		}
		else mod_file->sample[i] = NULL;
	}

}

void fix_mod_row_patterns(MODFILE *mod_file) {
	/* Need to fix row_pattern to clear entries past length since some players mess up with extra entries */
	const unsigned char valid_entries = mod_file->header.num_rows;
	memset(&mod_file->header.row_pattern[valid_entries], 0, 128 - valid_entries);
}

void write_mod_file(FILE *output_file, MODFILE *mod_file, SAMPLETYPE sample_type)
{
	int i, j, k;

	/* Need to fix endian order (MOD & 4MD use big endian) */
	for (i = 0; i < 31; i++)
	{
		mod_file->header.sample_entry[i].length = htons(mod_file->header.sample_entry[i].length);
		mod_file->header.sample_entry[i].repeat_start = htons(mod_file->header.sample_entry[i].repeat_start);
		mod_file->header.sample_entry[i].repeat_length = htons(mod_file->header.sample_entry[i].repeat_length);
	}

	/* Write out MOD header */
	fwrite(&(mod_file->header), sizeof(MODHEADER), 1, output_file);

	/* Reverse endian order again so sample data can be written later */
	for (i = 0; i < 31; i++)
	{
		mod_file->header.sample_entry[i].length = ntohs(mod_file->header.sample_entry[i].length);
		mod_file->header.sample_entry[i].repeat_start = ntohs(mod_file->header.sample_entry[i].repeat_start);
		mod_file->header.sample_entry[i].repeat_length = ntohs(mod_file->header.sample_entry[i].repeat_length);
	}

	/* Write out pattern data */
	for (i = 0; i < mod_file->num_patterns; i++)
		for (k = 0; k < PATTERN_LENGTH; k++)
			for (j = 0; j < NUM_CHANNELS; j++)
				write_note(output_file, &(mod_file->note[i][j][k]));

	/* Deallocate pattern buffers */
	for (i = 0; i < mod_file->num_patterns; i++)
	{
		for (j = 0; j < NUM_CHANNELS; j++) free(mod_file->note[i][j]);
		free(mod_file->note[i]);
	}
	free(mod_file->note);

	/* Write out samples */
	for (i = 0; i < MAX_SAMPLES; i++)
	{
		if (mod_file->sample[i] != NULL)
			if (sample_type == SAMPLE_TYPE_4BIT) {
				for (j = 0; j < mod_file->header.sample_entry[i].length; j++) {
					unsigned char buffer = (mod_file->sample[i][j * 2] & 0xF0) | (unsigned char)((mod_file->sample[i][j * 2 + 1] & 0xF0)) >> 4;
					fwrite(&buffer, 1, 1, output_file);
				}

			} else {
				fwrite(mod_file->sample[i], 1, mod_file->header.sample_entry[i].length * 2, output_file);
			}
	}

	/* Deallocate sample buffers */
	for (i = 0; i < MAX_SAMPLES; i++)
		if (mod_file->sample[i] != NULL) free(mod_file->sample[i]);
	free(mod_file->sample);
}

void convert_file(const char *input_filename, const char *output_filename, const SETTINGS* settings)
{
	FILE *input_file, *output_file;
	MODFILE mod_file;

	input_file = fopen(input_filename, "rb");
	if (input_file == NULL)
	{
		perror(input_filename);
		exit(EXIT_FAILURE);
	}

	output_file = fopen(output_filename, "wb");
	if (output_file == NULL)
	{
		perror(output_filename);
		exit(EXIT_FAILURE);
	}

	read_mod_file(input_file, &mod_file, settings->input_sample);

	if (settings->flags & FLAG_CLEAN_ROW_PATTERNS) {
		//This fixes playback issues with some mod players
		fix_mod_row_patterns(&mod_file);
	}

	write_mod_file(output_file, &mod_file, settings->output_sample);

	fclose(input_file);
	fclose(output_file);
}

void patch_extension(char *dest, const char *src, char *new_ext)
{
	char *ext_start;

	strcpy(dest, src);
	ext_start = strrchr(dest, '.');
	if (ext_start == NULL) ext_start = dest + strlen(dest);
	strcpy(ext_start, new_ext);
}

int main(int argc, char **argv)
{
	int i = 1;
	char output_filename[PATH_MAX];
	char output_extension[5];
	SETTINGS settings;
	memset(&settings, 0, sizeof(SETTINGS));

	if (argc < 3)
	{
		fprintf(stdout, "4MDConverter\n");
		fprintf(stdout, "Based on 4MD2MOD by Admiral_Bob - Updated by Litude\n");
		fprintf(stdout, "Convert between Alien Carnage/Halloween Harry 4MD and MOD music files\n");
		fprintf(stdout, "Usage: %s <-4mdtomod | -modto4md> [-clean] <FILE...>\n", argv[0]);
		fprintf(stdout, "NOTE: To fix broken playback on some players, use the -clean parameter\n");
		exit(EXIT_FAILURE);
	}

	while (i < argc) {
		if (argv[i][0] == '-') {
			if (!strcmp(argv[i], "-4mdtomod")) {
				settings.input_sample = SAMPLE_TYPE_4BIT;
				settings.output_sample = SAMPLE_TYPE_8BIT;
				strcpy(output_extension, ".mod");
			} else if (!strcmp(argv[i], "-modto4md")) {
				settings.input_sample = SAMPLE_TYPE_8BIT;
				settings.output_sample = SAMPLE_TYPE_4BIT;
				strcpy(output_extension, ".4md");
			} else if (!strcmp(argv[i], "-clean")) {
				settings.flags |= FLAG_CLEAN_ROW_PATTERNS;
			} else {
				fprintf(stderr, "Usage: %s <-4mdtomod | -modto4md> [-clean] <FILE...>\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		} else {
			break;
		}
		++i;
	}
	
	if (settings.input_sample == SAMPLE_TYPE_UNSPECIFIED || settings.output_sample == SAMPLE_TYPE_UNSPECIFIED || i == argc) {
		fprintf(stderr, "Usage: %s <-4mdtomod | -modto4md> [-clean] <FILE...>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	while (i < argc) {
		patch_extension(output_filename, argv[i], output_extension);
		printf("converting %s to %s\n", argv[i], output_filename);
		convert_file(argv[i], output_filename, &settings);
		++i;
	}

}
