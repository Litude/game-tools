#ifndef TEXTURES_H
#define TEXTURES_H

#include <stdint.h>
#include "bspfile.h"

#define MAXTEXTURENAME 16
#define MIPLEVELS 4
#define MAX_MAP_TEXTURES 512

typedef struct {
	uint32_t num_textures; // Number of BSPMIPTEX structures
	int32_t* texture_offsets;
} textureHeader;

typedef struct
{
	char name[MAXTEXTURENAME];  // Name of texture
	uint32_t width, height;     // Extends of the texture
	uint32_t miptex_offsets[MIPLEVELS]; // Offsets to texture mipmaps BSPMIPTEX;
} texture;

typedef struct
{
	textureHeader header;
	texture* textures;
} textureLump;

textureLump* create_textures_from_data(bspLump* textures_lump, uint32_t* count);
uint32_t trim_texture_lump(bspLump* tex_buffer, short* trimmed_indexes, char** trim_names, uint32_t trim_name_count);

#endif // !TEXTURES_H
