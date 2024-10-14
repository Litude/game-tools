#ifndef TEXINFO_H
#define TEXINFO_H

#include <stdint.h>
#include "vector.h"

#define TEXINFO_BINARY_SIZE 40
#define MAX_MAP_TEXINFO 8192

typedef struct
{
	float vecs[2][4]; // [s/t][xyz offset]
	uint32_t iMiptex; // Index into textures array
	uint32_t nFlags;  // Texture flags, seem to always be 0
} texInfo;

void texinfo_lump_to_buffer(uint32_t num_entries, texInfo* tex_info_lumps, bspLump* tex_buffer);
texInfo* create_texinfo_from_data(bspLump* tex_lump, uint32_t* count);
texInfo* trim_texinfo_offsets(texInfo* tex_infos, uint32_t num_tex_infos, uint16_t* trimmed_texture_indexes, uint32_t trimmed_texture_count, uint16_t* trimmed_texinfo_indexes, uint32_t* trimmed_texinfo_count);
void print_texinfo(texInfo* info);

#endif // !TEXINFO_H
