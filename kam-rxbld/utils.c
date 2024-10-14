#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint32_t getFileSize(FILE* filehandle) {
	fseek(filehandle, 0, SEEK_END);
	uint32_t fileSize = ftell(filehandle);
	fseek(filehandle, 0, SEEK_SET);

	return fileSize;
}
