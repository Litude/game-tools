#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bspfile.h"
#include "face.h"
#include "textures.h"
#include "texinfo.h"
#include "vector.h"
#include "vertex.h"
#include "color.h"
#include "scaling.h"
#include "qdbmp\qdbmp.h"

char* add_filename_suffix(const char* fullname) {
	const char* suffix = "_trimmed";

	char filename[255];
	char extension[255];

	filename[0] = '\0';
	extension[0] = '\0';

#ifdef WIN32
	char* separator = strrchr(fullname, '\\');
#else
	char* separator = strrchr(fullname, '/');
#endif

	if (separator) {
		strcpy(filename, separator+1);
	} else {
		strcpy(filename, fullname);
	}

	char* extension_ptr = strrchr(filename, '.');
	if (extension_ptr) {
		strcpy(extension, extension_ptr+1);
		*extension_ptr = '\0';
	}

	char* new_name = malloc(strlen(filename)+strlen(suffix)+1+strlen(extension)+1);
	new_name[0] = '\0';
	strcat(new_name, filename);
	strcat(new_name, suffix);
	strcat(new_name, ".");
	strcat(new_name, extension);
	
	return new_name;
}

static int cmp(void const* a, void const* b) {
	uint16_t first = *(uint16_t*)a;
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

int main (int argc, char *argv[])
{
	short trimmed_texture_indexes[MAX_MAP_TEXTURES];
	short* trimmed_texinfo_indexes = malloc(sizeof(short) * MAX_MAP_TEXINFO);
	//short trimmed_texinfo_indexes[MAX_MAP_TEXINFO];
	printf("BSPTexOPT v1.0\n");
	if (argc < 2) {
		printf("Removes textures that are embedded in a GoldSrc BSP file.\n");
		printf("Usage: %s <bspfile>\n", argv[0]);
		return 1;
	}
	FILE* bsp_file = fopen(argv[1], "rb");
	if (!bsp_file) {
		perror("Failed opening file, error occured");
		return 1;
	}
	printf("Reading BSP file %s...\n", argv[1]);
	bspFile* input = read_bsp_file(bsp_file);
	fclose(bsp_file);

	if (input->version == -1) {
		printf("Not a valid BSP file, terminating...\n");
		free(input);
		return 1;
	}

	uint32_t vertex_count = 0;
	vertex* vertices = create_vertices_from_data(&input->lump[LUMP_VERTICES], &vertex_count);
	printf("Vertices created\n");
	uint32_t tex_info_count = 0;
	texInfo* tex_info = create_texinfo_from_data(&input->lump[LUMP_TEXINFO], &tex_info_count);
	printf("Texinfo created\n");
	uint32_t face_count = 0;
	face* faces = create_faces_from_data(&input->lump[LUMP_FACES], &face_count);
	printf("Faces created\n");
	face* trimmed_faces = malloc(face_count * sizeof(face));
	memcpy(trimmed_faces, faces, face_count * sizeof(face));

	uint32_t trimmed_texture_count = trim_texture_lump(&input->lump[LUMP_TEXTURES], trimmed_texture_indexes, argv + 2, argc - 2);
	uint32_t trimmed_texinfo_count = 0;
	texInfo* trimmed_tex_info = trim_texinfo_offsets(tex_info, tex_info_count, trimmed_texture_indexes, trimmed_texture_count, trimmed_texinfo_indexes, &trimmed_texinfo_count);

	printf("Trimmed %d textures\n", trimmed_texture_count);

	//printf("Num edges: %d\n", input->lump[LUMP_EDGES].length / 4);

	//printf("Stuff: %d\n", input->lump[LUMP_SURFEDGES].data[0]);
	//printf("Texinfo count: %d\n", tex_info_count);
	//printf("Vertex count: %d\n", vertex_count);

	uint32_t lightLumpSize = input->lump[LUMP_LIGHTING].length;
	char* newLights = malloc(lightLumpSize);
	//memset(newLights, 0, input->lump[LUMP_LIGHTING].length);
	memcpy(newLights, input->lump[LUMP_LIGHTING].data, input->lump[LUMP_LIGHTING].length);

	uint32_t curOffset = 0;

	for (uint32_t i = 0; i < face_count; ++i) {
		//printf("Texture info: %d\n", faces[i].iTextureInfo);
		int32_t lightMapCount = get_light_map_count(&faces[i]);
		if (lightMapCount > 0) {
			trimmed_faces[i].nLightmapOffset = curOffset;
			int size[2] = { 0, 0 };
			if (!GetFaceLightmapSize(&faces[i], &tex_info[faces[i].iTextureInfo], (vertex*)input->lump[LUMP_VERTICES].data, (edge*)input->lump[LUMP_EDGES].data, (int32_t*)input->lump[LUMP_SURFEDGES].data, size)) {
				printf("Calculation failed!\n");
			}
			v2d original_lsize;
			original_lsize.x = size[0];
			original_lsize.y = size[1];
			uint32_t light_map_size = original_lsize.x * original_lsize.y * 3;

			for (int32_t j = 0; j < lightMapCount; ++j) {
				//BMP* lightmap = BMP_Create(original_lsize.x, original_lsize.y, 32);
				//for (int32_t y = 0; y < original_lsize.y; ++y) {
				//	for (int32_t x = 0; x < original_lsize.x; ++x) {
				//		uint8_t* dataStart = input->lump[LUMP_LIGHTING].data + faces[i].nLightmapOffset + light_map_size * j;
				//		uint8_t red = dataStart[(y * original_lsize.x + x) * 3];
				//		uint8_t green = dataStart[(y * original_lsize.x + x) * 3 + 1];
				//		uint8_t blue = dataStart[(y * original_lsize.x + x) * 3 + 2];
				//		BMP_SetPixelRGB(lightmap, x, y, red, green, blue);
				//	}
				//}
				//char filename[255];
				//sprintf(filename, "face%05d_l%d.bmp", i, j);

				//BMP_WriteFile(lightmap, filename);
				//BMP_Free(lightmap);
				//lightmap = NULL;

				//v2d original_lsize = calculate_lightmap_size(&faces[i], &tex_info[faces[i].iTextureInfo], (vertex*)input->lump[LUMP_VERTICES].data, (edge*)input->lump[LUMP_EDGES].data, (int32_t*)input->lump[LUMP_SURFEDGES].data);
				//if (bsearch(&faces[i].iTextureInfo, trimmed_texinfo_indexes, trimmed_texinfo_count, sizeof(uint16_t), cmp)) {
				//	v2d trimmed_lsize = calculate_lightmap_size(&faces[i], &trimmed_tex_info[faces[i].iTextureInfo], (vertex*)input->lump[LUMP_VERTICES].data, (edge*)input->lump[LUMP_EDGES].data, (int32_t*)input->lump[LUMP_SURFEDGES].data);
				//	if (original_lsize.x != trimmed_lsize.x || original_lsize.y != trimmed_lsize.y) {
				//		//printf("Found scaling stuff\n");
				//		//memset(newLights + faces[i].nLightmapOffset, 0, original_lsize.x * original_lsize.y * 3);
				//		scale_image(i, (color*)(input->lump[LUMP_LIGHTING].data + faces[i].nLightmapOffset), (color*)(newLights + faces[i].nLightmapOffset), original_lsize.x, original_lsize.y, trimmed_lsize.x, trimmed_lsize.y);
				//		//curOffset += trimmed_lsize.x * trimmed_lsize.y * 3;
				//		continue;
				//	}
				//}
				uint32_t nextOffset = curOffset + light_map_size;
				if (nextOffset > lightLumpSize) {
					printf("Reallocating lights...\n");
					newLights = realloc(newLights, nextOffset);
				}

				memcpy(newLights + trimmed_faces[i].nLightmapOffset + light_map_size * j, input->lump[LUMP_LIGHTING].data + faces[i].nLightmapOffset + light_map_size * j, light_map_size);
				curOffset = nextOffset;
			}
		}

	}

	texinfo_lump_to_buffer(tex_info_count, trimmed_tex_info, &input->lump[LUMP_TEXINFO]);
	printf("Old lights lump size %d, new size %d...\n", input->lump[LUMP_LIGHTING].length, curOffset);
	input->lump[LUMP_LIGHTING].length = curOffset;
	input->lump[LUMP_LIGHTING].data = newLights;

	//memcpy(input->lump[LUMP_LIGHTING].data, newLights, input->lump[LUMP_LIGHTING].length);
	memcpy(input->lump[LUMP_FACES].data, trimmed_faces, face_count * sizeof(face));

	//printf("Edge size: %d\n", sizeof(edge));
	//printf("Vertex size: %d\n", sizeof(vertex));

	char* output_name = add_filename_suffix(argv[1]);

	printf("Writing BSP file %s...\n", output_name);
	FILE* output_file = fopen(output_name, "wb");
	write_bsp_file(output_file, input);
	fclose(output_file);

	free_bsp_file(input);
	free(output_name);
	
	free(trimmed_texinfo_indexes);

	free(trimmed_faces);
	free(trimmed_tex_info);
	//free(newLights);
	free(vertices);
	free(tex_info);
	free(faces);

	return 0;
}
