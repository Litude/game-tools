#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <math.h>
#include "bspplane.h"
#include "bspfile.h"
#include "face.h"
#include "textures.h"
#include "texinfo.h"
#include "vector.h"
#include "vertex.h"
#include "color.h"
#include "scaling.h"
#include "qdbmp\qdbmp.h"

#define PI 3.141592f


static const vec3  s_baseaxis[18] = {
	{0, 0, 1}, {1, 0, 0}, {0, -1, 0},                      // floor
	{0, 0, -1}, {1, 0, 0}, {0, -1, 0},                     // ceiling
	{1, 0, 0}, {0, 1, 0}, {0, 0, -1},                      // west wall
	{-1, 0, 0}, {0, 1, 0}, {0, 0, -1},                     // east wall
	{0, 1, 0}, {1, 0, 0}, {0, 0, -1},                      // south wall
	{0, -1, 0}, {1, 0, 0}, {0, 0, -1},                     // north wall
};

float dotProduct(const vec3* v1, const vec3* v2)
{
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

int TextureAxisFromPlane(const BSPPLANE* pln, vec3* xv, vec3* yv)
{
	int             bestaxis;
	float           dot, best;
	int             i;

	best = 0;
	bestaxis = 0;

	for (i = 0; i < 6; i++)
	{
		dot = dotProduct(&pln->vNormal, &s_baseaxis[i * 3]);
		//dot = CalculatePointVecsProduct((float*)&pln->vNormal.x, (float*)&s_baseaxis[i * 3].x);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}

	*xv = s_baseaxis[bestaxis * 3 + 1];
	*yv = s_baseaxis[bestaxis * 3 + 2];

	return bestaxis;
}

float AngleFromTextureAxis(float x, float y, float z, bool x_coord, int type)
{
	float retval = 0.0f;

	if (type < 2)
	{
		if (x_coord)
		{
			return -1.f * atan2(y, x) * (180.f / PI);
		}
		else
		{
			return atan2(x, y) * (180.f / PI);
		}
	}


	if (type < 4)
	{
		if (x_coord)
		{
			return -1.f * atan2(z, y) * (180.f / PI);
		}
		else
		{
			return atan2(y, z) * (180.f / PI);
		}
	}

	if (type < 6)
	{
		if (x_coord)
		{
			return -1.f * atan2(z, x) * (180.f / PI);
		}
		else
		{
			return atan2(x, z) * (180.f / PI);
		}
	}


	return retval;
}

static int approximateAngle(int angle) {
	int smallest_difference = INT_MAX;
	int best_angle = -360;
	for (int test_angle = -360; test_angle <= 360; test_angle += 90) {
		int difference = (angle - test_angle);
		int diffSquare = difference * difference;
		if (diffSquare < smallest_difference) {
			best_angle = test_angle;
			smallest_difference = diffSquare;
		}
	}
	return best_angle;
}

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
	bool exporting = false;
	int32_t filename_arg = 2;
	mkdir("export");
	mkdir("import");
	mkdir("import_transform");
	printf("BSPLitReplace v1.0\n");
	if (argc < 3) {
		printf("Imports/exports lightmaps from a a GoldSrc BSP file.\n");
		printf("Usage: %s [-import/-export] [-transform] <bspfile>\n", argv[0]);
		return 1;
	}
	if (!strcmp(argv[1], "-import")) {
		exporting = false;
	}
	else if (!strcmp(argv[1], "-export")) {
		exporting = true;
		char directoryName[255];
		sprintf(directoryName, "export/%s", argv[2]);

		mkdir(directoryName);
	}
	else {
		printf("Imports/exports lightmaps from a a GoldSrc BSP file.\n");
		printf("Usage: %s [-import/-export] <bspfile>\n", argv[0]);
		return 1;
	}

	bool transform_import = false;

	if (!strcmp(argv[2], "-transform")) {
		if (exporting) {
			printf("-transform only supported when importing!\n");
			return 1;
		}
		++filename_arg;
		transform_import = true;
	}

	FILE* bsp_file = fopen(argv[filename_arg], "rb");
	if (!bsp_file) {
		perror("Failed opening file, error occured");
		return 1;
	}
	printf("Reading BSP file %s...\n", argv[filename_arg]);
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

	FILE* importLog = NULL;

	char filename[255];
	if (!exporting && transform_import) {
		sprintf(filename, "import_transform/actionlog.txt");
		importLog = fopen(filename, "w");
	}

	int32_t direct_import_count = 0;
	int32_t transform_import_count = 0;
	int32_t skip_import_count = 0;

	for (uint32_t i = 0; i < face_count; ++i) {
		//printf("Texture info: %d\n", faces[i].iTextureInfo);
		int32_t lightMapCount = get_light_map_count(&faces[i]);
		if (lightMapCount > 0) {
			trimmed_faces[i].nLightmapOffset = curOffset;
			int size[2] = { 0, 0 };
			texInfo* textureInfo = &tex_info[faces[i].iTextureInfo];
			if (!GetFaceLightmapSize(&faces[i], textureInfo, (vertex*)input->lump[LUMP_VERTICES].data, (edge*)input->lump[LUMP_EDGES].data, (int32_t*)input->lump[LUMP_SURFEDGES].data, size)) {
				printf("Calculation failed!\n");
			}
			v2d original_lsize;
			original_lsize.x = size[0];
			original_lsize.y = size[1];
			uint32_t light_map_size = original_lsize.x * original_lsize.y * 3;

			for (int32_t j = 0; j < lightMapCount; ++j) {

				if (exporting) {
					BMP* lightmap = BMP_Create(original_lsize.x, original_lsize.y, 32);
					for (int32_t y = 0; y < original_lsize.y; ++y) {
						for (int32_t x = 0; x < original_lsize.x; ++x) {
							uint8_t* dataStart = input->lump[LUMP_LIGHTING].data + faces[i].nLightmapOffset + light_map_size * j;
							uint8_t red = dataStart[(y * original_lsize.x + x) * 3];
							uint8_t green = dataStart[(y * original_lsize.x + x) * 3 + 1];
							uint8_t blue = dataStart[(y * original_lsize.x + x) * 3 + 2];
							BMP_SetPixelRGB(lightmap, x, y, red, green, blue);
						}
					}
					sprintf(filename, "export/%s/face%05d_l%d.bmp", argv[filename_arg], i, j);

					BMP_WriteFile(lightmap, filename);

					sprintf(filename, "export/%s/face%05d_l%d.txt", argv[filename_arg], i, j);
					FILE* textFile = fopen(filename, "w");
					BSPPLANE* planes = (BSPPLANE*)input->lump[LUMP_PLANES].data;

					vec3 xv, yv;
					int planeType = TextureAxisFromPlane(&planes[faces[i].iPlane], &xv, &yv);
					float angleX = AngleFromTextureAxis(textureInfo->vecs[0][0], textureInfo->vecs[0][1], textureInfo->vecs[0][2], true, planeType);
					float angleY = AngleFromTextureAxis(textureInfo->vecs[1][0], textureInfo->vecs[1][1], textureInfo->vecs[1][2], false, planeType);
					fprintf(textFile, "u: %f %f %f (%f)\n", textureInfo->vecs[0][0], textureInfo->vecs[0][1], textureInfo->vecs[0][2], angleX);
					fprintf(textFile, "v: %f %f %f (%f)\n", textureInfo->vecs[1][0], textureInfo->vecs[1][1], textureInfo->vecs[1][2], angleY);
					fprintf(textFile, "s: %f %f\n", textureInfo->vecs[0][3], textureInfo->vecs[1][3]);
					lightmap = NULL;
					fclose(textFile);
				}
				else if (!transform_import) {
					sprintf(filename, "import/face%05d_l%d.bmp", i, j);
					BMP* importMap = BMP_ReadFile(filename);
					if (importMap) {
						uint32_t bmpWidth = BMP_GetWidth(importMap);
						uint32_t bmpHeight = BMP_GetHeight(importMap);
						if (bmpWidth == original_lsize.x && bmpHeight == original_lsize.y) {
							printf("Importing face %d lightmap %d\n", i, j);
							for (int32_t y = 0; y < original_lsize.y; ++y) {
								for (int32_t x = 0; x < original_lsize.x; ++x) {
									uint8_t red = 0;
									uint8_t green = 0;
									uint8_t blue = 0;
									BMP_GetPixelRGB(importMap, x, y, &red, &green, &blue);
									uint8_t* dataStart = input->lump[LUMP_LIGHTING].data + faces[i].nLightmapOffset + light_map_size * j;
									dataStart[(y * original_lsize.x + x) * 3] = red;
									dataStart[(y * original_lsize.x + x) * 3 + 1] = green;
									dataStart[(y * original_lsize.x + x) * 3 + 2] = blue;
								}
							}
						}
						else {
							printf("Skipping face %d lightmap %d import due to size mismatch (got %dx%d, expected %dx%d)\n", i, j, bmpWidth, bmpHeight, original_lsize.x, original_lsize.y);
						}
					}
					else {
						if (BMP_GetError() != BMP_FILE_NOT_FOUND) {
							printf("Opening %s failed: %s\n", filename, BMP_GetErrorDescription());
						}
					}
				}
				else if (!exporting) {
					printf("Processing face %d\n", i);
					sprintf(filename, "import_transform/face%05d_l%d.bmp", i, j);
					BMP* importMap = BMP_ReadFile(filename);
					if (importMap) {
						sprintf(filename, "import_transform/face%05d_l%d.txt", i, j);
						FILE* textFile = fopen(filename, "r");
						float dummy1 = 0.0f, dummy2 = 0.0f, dummy3 = 0.0f, importAngleX = 0.0f, importAngleY = 0.0f;
						fscanf(textFile, "u: %f %f %f (%f) ", &dummy1, &dummy2, &dummy3, &importAngleX);
						//printf("Read: u %f %f %f (%f)\n", dummy1, dummy2, dummy3, importAngleX);
						fscanf(textFile, "v: %f %f %f (%f) ", &dummy1, &dummy2, &dummy3, &importAngleY);
						//printf("Read: v %f %f %f (%f)\n", dummy1, dummy2, dummy3, importAngleY);
						fclose(textFile);

						int importAngleXi = lroundf(importAngleX);
						int importAngleYi = lroundf(importAngleY);


						BSPPLANE* planes = (BSPPLANE*)input->lump[LUMP_PLANES].data;

						vec3 xv, yv;
						int planeType = TextureAxisFromPlane(&planes[faces[i].iPlane], &xv, &yv);
						float angleX = AngleFromTextureAxis(textureInfo->vecs[0][0], textureInfo->vecs[0][1], textureInfo->vecs[0][2], true, planeType);
						float angleY = AngleFromTextureAxis(textureInfo->vecs[1][0], textureInfo->vecs[1][1], textureInfo->vecs[1][2], false, planeType);

						int currentAngleXi = lroundf(angleX);
						int currentAngleYi = lroundf(angleY);


						int angleYDelta = importAngleYi - currentAngleYi;

						char actionString[255] = "";
						sprintf(actionString, "Face %d l%d action: ", i, j);

						if (angleYDelta % 90 != 0) {
							int approxYDelta = approximateAngle(angleYDelta);
							sprintf(actionString + strlen(actionString), "approx rot. %d degrees (%d original), ", approxYDelta, angleYDelta);
							angleYDelta = approxYDelta;
						}
						angleYDelta %= 360;

						int angleXDiff = currentAngleYi - importAngleYi;
						if (angleXDiff % 90 != 0) {
							int approxXDiff= approximateAngle(angleXDiff);
							sprintf(actionString + strlen(actionString), "approx flip %d degrees (%d original), ", approxXDiff, angleXDiff);
							angleXDiff = approxXDiff;
						}
						angleXDiff %= 360;

						int angleXDeltaFull = ((angleXDiff + importAngleXi) - currentAngleXi) % 360;
						//int angleXDeltaFull = (((-angleYDelta) + importAngleXi) - currentAngleXi) % 360;
						//int angleXDeltaFull = (((currentAngleYi - importAngleYi) + importAngleXi) - currentAngleXi) % 360;

						int angleXDelta = angleXDeltaFull;
						if (angleXDeltaFull >= 180) {
							angleXDelta = -180 + (angleXDeltaFull - 180);
						}
						else if (angleXDeltaFull < -180) {
							angleXDelta = 180 + (angleXDeltaFull + 180);
						}

						bool shouldRotate = false;
						bool hasAction = false;
						bool transformError = false;
						int flipType = 0;

						BMP* transformed = importMap;


						if (angleYDelta != 0) {
							hasAction = true;
							sprintf(actionString + strlen(actionString), "rotate %d degrees, ", angleYDelta);
							BMP* rotated = BMP_Rotated(transformed, angleYDelta);
							if (rotated) {
								BMP_Free(transformed);
								transformed = rotated;
							}
							else {
								fprintf(importLog, "Rotating %d degrees failed!\n", angleYDelta);
								transformError = true;
							}
						}

						if (angleXDelta != 0) {
							hasAction = true;
							int roundedAngle = approximateAngle(llroundf(angleY)) % 360;
							if (roundedAngle == 90 || roundedAngle == -90) {
								flipType = 1;
								sprintf(actionString + strlen(actionString), "flip vertically, ");
								BMP* flipped = BMP_FlippedVertically(transformed);
								if (flipped) {
									BMP_Free(transformed);
									transformed = flipped;
								}
								else {
									transformError = true;
									fprintf(importLog, "Flipping vertically failed!\n");
								}
							}
							else if (roundedAngle == 0 || roundedAngle == 180) {
								sprintf(actionString + strlen(actionString), "flip horizontally, ");
								flipType = 2;
								BMP* flipped = BMP_FlippedHorizontally(transformed);
								if (flipped) {
									BMP_Free(transformed);
									transformed = flipped;
								}
								else {
									transformError = true;
									fprintf(importLog, "Flipping horizontally failed!\n");
								}
							}
							else {
								sprintf(actionString + strlen(actionString), "flip unknown, ");
								//fprintf(importLog, "Flipping failed, type undetermined!\n");
								//flipType = 3;
								//transformError = true;
							}
						}

						uint32_t bmpWidth = BMP_GetWidth(transformed);
						uint32_t bmpHeight = BMP_GetHeight(transformed);

						if (bmpWidth != original_lsize.x || bmpHeight != original_lsize.y) {
							hasAction = true;
							sprintf(actionString + strlen(actionString), "scale from %dx%d to %dx%d, ", bmpWidth, bmpHeight, original_lsize.x, original_lsize.y);
							BMP* scaled = BMP_Scaled(transformed, original_lsize.x, original_lsize.y);
							if (scaled) {
								BMP_Free(transformed);
								transformed = scaled;
							}
							else {
								transformError = true;
								fprintf(importLog, "Scaling from from %dx%d to %dx%d failed!\n", bmpWidth, bmpHeight, original_lsize.x, original_lsize.y);
							}
						}

						if (!hasAction) {
							sprintf(actionString + strlen(actionString), "nothing ");
							++direct_import_count;
						}
						sprintf(actionString + strlen(actionString), "\n");

						if (hasAction) {
							fprintf(importLog, actionString);
							++transform_import_count;
						}

						if (transformError) {
							++skip_import_count;
						}
						//fprintf(importLog, "Face %d l%d action: rotate %d degrees, flip: %d, scale from %dx%d to %dx%d\n", i, j, angleYDelta, angleXDelta != 0, bmpWidth, bmpHeight, original_lsize.x, original_lsize.y);

						fflush(importLog);
						if (!transformError) {
							printf("Importing face %d lightmap %d\n", i, j);
							for (int32_t y = 0; y < original_lsize.y; ++y) {
								for (int32_t x = 0; x < original_lsize.x; ++x) {
									uint8_t red = 0;
									uint8_t green = 0;
									uint8_t blue = 0;
									BMP_GetPixelRGB(transformed, x, y, &red, &green, &blue);
									uint8_t* dataStart = input->lump[LUMP_LIGHTING].data + faces[i].nLightmapOffset + light_map_size * j;
									dataStart[(y * original_lsize.x + x) * 3] = red;
									dataStart[(y * original_lsize.x + x) * 3 + 1] = green;
									dataStart[(y * original_lsize.x + x) * 3 + 2] = blue;
								}
							}
						}
						else {
							printf("Skipping face %d lightmap %d import due to size mismatch (got %dx%d, expected %dx%d)\n", i, j, bmpWidth, bmpHeight, original_lsize.x, original_lsize.y);
						}
						BMP_Free(transformed);
					}
				}


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

	if (!exporting && transform_import) {
		fprintf(importLog, "Result:\n%d direct imports\n%d transformed imports\n%d skipped\n", direct_import_count, transform_import_count, skip_import_count);
		fclose(importLog);
	}

	texinfo_lump_to_buffer(tex_info_count, trimmed_tex_info, &input->lump[LUMP_TEXINFO]);
	printf("Old lights lump size %d, new size %d...\n", input->lump[LUMP_LIGHTING].length, curOffset);
	input->lump[LUMP_LIGHTING].length = curOffset;
	input->lump[LUMP_LIGHTING].data = newLights;

	//memcpy(input->lump[LUMP_LIGHTING].data, newLights, input->lump[LUMP_LIGHTING].length);
	memcpy(input->lump[LUMP_FACES].data, trimmed_faces, face_count * sizeof(face));

	//printf("Edge size: %d\n", sizeof(edge));
	//printf("Vertex size: %d\n", sizeof(vertex));

	char* output_name = add_filename_suffix(argv[filename_arg]);

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
