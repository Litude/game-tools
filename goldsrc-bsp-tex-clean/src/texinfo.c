#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bspfile.h"
#include "texinfo.h"

void texinfo_lump_to_buffer(uint32_t num_entries, texInfo* tex_info_lumps, bspLump* tex_buffer) {
	for (uint32_t i = 0; i < num_entries; ++i) {
		for (uint32_t j = 0; j < 8; ++j) {
			memcpy(&tex_buffer->data[TEXINFO_BINARY_SIZE * i + j * 4], &tex_info_lumps[i].vecs[j / 4][j % 4], sizeof(float));
		}
		memcpy(&tex_buffer->data[TEXINFO_BINARY_SIZE * i + 32], &tex_info_lumps[i].iMiptex, sizeof(uint32_t));
		memcpy(&tex_buffer->data[TEXINFO_BINARY_SIZE * i + 36], &tex_info_lumps[i].nFlags, sizeof(uint32_t));
	}
}

static int cmp(void const* a, void const* b) {
	uint16_t first = *(uint16_t *)a;
	uint16_t second = *(uint16_t*)b;

	if (first > second) {
		return 1;
	}
	else if (first == second) {
		return 0;
	}
	else {
		return -1;
	}
}

texInfo* create_texinfo_from_data(bspLump* tex_lump, uint32_t* count) {
	const uint32_t num_tex_infos = tex_lump->length / TEXINFO_BINARY_SIZE;
	texInfo* tex_infos = malloc(num_tex_infos * sizeof(texInfo));
	if (tex_infos) {
		for (uint32_t i = 0; i < num_tex_infos; ++i) {
			for (uint32_t j = 0; j < 8; ++j) {
				memcpy(&(tex_infos[i].vecs[j / 4][j % 4]), &(tex_lump->data[TEXINFO_BINARY_SIZE * i + j * 4]), sizeof(float));
			}
			memcpy(&(tex_infos[i].iMiptex), &(tex_lump->data[TEXINFO_BINARY_SIZE * i + 32]), sizeof(uint32_t));
			memcpy(&(tex_infos[i].nFlags), &(tex_lump->data[TEXINFO_BINARY_SIZE * i + 36]), sizeof(uint32_t));
		}
		*count = num_tex_infos;
	}
	return tex_infos;
}

// creates a trimmed copy of texinfo
texInfo* trim_texinfo_offsets(texInfo* tex_infos, uint32_t num_tex_infos, uint16_t* trimmed_texture_indexes, uint32_t trimmed_texture_count, uint16_t* trimmed_texinfo_indexes, uint32_t* trimmed_texinfo_count) {
	texInfo* trimmed_tex = malloc(num_tex_infos * sizeof(texInfo));
	if (trimmed_tex) {
		memcpy(trimmed_tex, tex_infos, num_tex_infos * sizeof(texInfo));
		//for (uint32_t i = 0; i < num_tex_infos; ++i) {
		//	uint16_t texture_index = (uint16_t)trimmed_tex[i].iMiptex;
		//	if (bsearch(&texture_index, trimmed_texture_indexes, trimmed_texture_count, sizeof(uint16_t), cmp)) {
		//		for (uint32_t j = 0; j < 8; ++j) {
		//			trimmed_tex[i].vecs[j / 4][j % 4] /= 2;
		//		}
		//		trimmed_texinfo_indexes[*trimmed_texinfo_count] = i;
		//		++(*trimmed_texinfo_count);
		//	}
		//}
	}
	return trimmed_tex;
}


//void trim_texinfo_offsets(bspLump* tex_lump, uint16_t* trimmed_indexes, uint32_t trimmed_count) {
//	const uint32_t num_tex_infos = tex_lump->length / TEXINFO_BINARY_SIZE;
//	texInfo* tex_infos = malloc(num_tex_infos * sizeof(texInfo));
//	if (tex_infos) {
//		for (uint32_t i = 0; i < num_tex_infos; ++i) {
//			for (uint32_t j = 0; j < 8; ++j) {
//				memcpy(&(tex_infos[i].vecs[j / 4][j % 4]), &(tex_lump->data[TEXINFO_BINARY_SIZE * i + j * 4]), sizeof(float));
//			}
//			memcpy(&tex_infos[i].iMiptex, &(tex_lump->data[TEXINFO_BINARY_SIZE * i + 32]), sizeof(uint32_t));
//			memcpy(&tex_infos[i].nFlags, &(tex_lump->data[TEXINFO_BINARY_SIZE * i + 36]), sizeof(uint32_t));
//		}
//
//		for (uint32_t i = 0; i < num_tex_infos; ++i) {
//			uint16_t texture_index = (uint16_t)tex_infos[i].iMiptex;
//			if (bsearch(&texture_index, trimmed_indexes, trimmed_count, sizeof(uint16_t), cmp)) {
//				for (uint32_t j = 0; j < 8; ++j) {
//					tex_infos[i].vecs[j / 4][j % 4] /= 2;
//				}
//			}
//		}
//
//		lump_to_buffer(num_tex_infos, tex_infos, tex_lump);
//
//		free(tex_infos);
//	}
//}

void print_texinfo(texInfo* info) {
	printf("Texture info:\n");
	printf("vS %fx, %fy, %fz, %fo\n", info->vecs[0][0], info->vecs[0][1], info->vecs[0][2], info->vecs[0][3]);
	printf("vT %fx, %fy, %fz, %fo\n", info->vecs[1][0], info->vecs[1][1], info->vecs[1][2], info->vecs[1][3]);
	printf("Miptext %d, flags %d\n", info->iMiptex, info->nFlags);
}