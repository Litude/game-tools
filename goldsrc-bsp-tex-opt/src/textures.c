#include <string.h>
#include <stdlib.h>
#include "textures.h"

static uint32_t update_offsets(textureLump* tex_lump) {
	uint32_t empty_textures = 0;
	for (uint32_t i = 0; i < tex_lump->header.num_textures; ++i) {
		if (tex_lump->textures[i].name[0]) {
			tex_lump->header.texture_offsets[i] = sizeof(uint32_t) + sizeof(uint32_t) * tex_lump->header.num_textures + sizeof(texture) * (i - empty_textures);
		}
		else {
			++empty_textures;
			tex_lump->header.texture_offsets[i] = -1;
		}
	}
	return empty_textures;
}

static void lump_to_buffer(textureLump* tex_lump, bspLump* tex_buffer) {
	memcpy(tex_buffer->data, &tex_lump->header.num_textures, sizeof(uint32_t));
	for (uint32_t i = 0; i < tex_lump->header.num_textures; ++i) {
		memcpy(&tex_buffer->data[sizeof(int32_t) * i] + sizeof(uint32_t), &tex_lump->header.texture_offsets[i], sizeof(int32_t));
	}

	uint32_t header_size = tex_lump->header.num_textures * sizeof(int32_t) + sizeof(uint32_t);
	for (uint32_t i = 0; i < tex_lump->header.num_textures; ++i) {
		memcpy(&tex_buffer->data[header_size+i*sizeof(texture)], &tex_lump->textures[i], sizeof(texture));
	}
}

textureLump* create_textures_from_data(bspLump* textures_lump, uint32_t* count) {
	textureLump* tex_lump = malloc(sizeof(textureLump));
	if (tex_lump) {
		memcpy(&(tex_lump->header.num_textures), textures_lump->data, sizeof(int32_t));
		tex_lump->header.texture_offsets = malloc(tex_lump->header.num_textures * sizeof(int32_t));
		memcpy(tex_lump->header.texture_offsets, textures_lump->data + sizeof(int32_t), tex_lump->header.num_textures * sizeof(int32_t));
		tex_lump->textures = malloc(tex_lump->header.num_textures * sizeof(texture));

		for (uint32_t i = 0; i < tex_lump->header.num_textures; ++i) {
			memcpy(&(tex_lump->textures[i].name), &(textures_lump->data[tex_lump->header.texture_offsets[i]]), MAXTEXTURENAME);
			memcpy(&(tex_lump->textures[i].width), &(textures_lump->data[tex_lump->header.texture_offsets[i]]) + MAXTEXTURENAME, sizeof(uint32_t));
			memcpy(&(tex_lump->textures[i].height), &(textures_lump->data[tex_lump->header.texture_offsets[i]]) + MAXTEXTURENAME + 4, sizeof(uint32_t));
			memcpy(&(tex_lump->textures[i].miptex_offsets), &(textures_lump->data[tex_lump->header.texture_offsets[i]]) + MAXTEXTURENAME + 8, sizeof(uint32_t) * MIPLEVELS);
		}
		*count = 1;

	}
	return tex_lump;
}

uint32_t trim_texture_lump(bspLump* tex_buffer, short* trimmed_indexes, char** trim_names, uint32_t trim_name_count) {
	textureLump tex_lump;
	memcpy(&(tex_lump.header.num_textures), tex_buffer->data, sizeof(int32_t));
	tex_lump.header.texture_offsets = malloc(tex_lump.header.num_textures * sizeof(int32_t));

	memcpy(tex_lump.header.texture_offsets, tex_buffer->data+sizeof(int32_t), tex_lump.header.num_textures * sizeof(int32_t));

	tex_lump.textures = malloc(tex_lump.header.num_textures * sizeof(texture));

	for (uint32_t i = 0; i < tex_lump.header.num_textures; ++i) {
		if (tex_lump.header.texture_offsets[i] != -1) {
			memcpy(&(tex_lump.textures[i].name), &(tex_buffer->data[tex_lump.header.texture_offsets[i]]), 16);
			memcpy(&(tex_lump.textures[i].width), &(tex_buffer->data[tex_lump.header.texture_offsets[i]]) + 16, sizeof(uint32_t));
			memcpy(&(tex_lump.textures[i].height), &(tex_buffer->data[tex_lump.header.texture_offsets[i]]) + 16 + sizeof(uint32_t), sizeof(uint32_t));
			//set all miptex offsets to zero since they will be removed anyway
			memset(tex_lump.textures[i].miptex_offsets, 0, sizeof(uint32_t) * MIPLEVELS);
		}
		else {
			tex_lump.textures[i].name[0] = '\0';
		}
	}

	uint32_t trimmed_amount = 0;

	for (uint32_t i = 0; i < tex_lump.header.num_textures; ++i) {
		for (uint32_t j = 0; j < trim_name_count; ++j) {
			//printf("Trim name count: %d\n", trim_name_count);
			if (!stricmp(tex_lump.textures[i].name, trim_names[j])) {
				//printf("Trimmed %s\n", trim_names[j]);
				trimmed_indexes[trimmed_amount] = i;
				tex_lump.textures[i].width /= 2;
				tex_lump.textures[i].height /= 2;
				++trimmed_amount;
				break;
			}
		}
		//if (strcmp(tex_lump.textures[i].name, "zs_lastride_03") && (tex_lump.textures[i].width >= 256 || tex_lump.textures[i].height >= 256)) {
		//	trimmed_indexes[trimmed_amount] = i;
		//	tex_lump.textures[i].width /= 2;
		//	tex_lump.textures[i].height /= 2;
		//	++trimmed_amount;
		//}
	}

	uint32_t empty_textures = update_offsets(&tex_lump);

	bspLump* new_buffer;
	new_buffer = malloc(sizeof(bspLump));
	new_buffer->length = (tex_lump.header.num_textures * sizeof(int32_t)) + ((tex_lump.header.num_textures - empty_textures) * sizeof(texture)) + sizeof(uint32_t);
	if (tex_buffer->length == new_buffer->length) {
		printf("WARNING: Size of old and new texture lump matches; there are probably no textures included in the original BSP file.\n");
	} else {
		printf("Old texture lump length: %d\n", tex_buffer->length);
		printf("New texture lump length: %d\n", new_buffer->length);
	}

	memset(&new_buffer->offset, 0, sizeof(int32_t));

	new_buffer->data = malloc(new_buffer->length);
	lump_to_buffer(&tex_lump, new_buffer);

	free(tex_lump.header.texture_offsets);
	free(tex_lump.textures);

	memcpy(tex_buffer->data, new_buffer->data, new_buffer->length);
	tex_buffer->length = new_buffer->length;

	free(new_buffer->data);
	free(new_buffer);

	return trimmed_amount;
}