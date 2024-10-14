#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include "bspfile.h"
#include "face.h"
#include "vertex.h"

typedef float vec_t;
typedef struct
{
	float x, y, z;
} vec3;

#define TEXTURE_STEP        16
#define MAX_SURFACE_EXTENT  64

face* create_faces_from_data(bspLump* lump, uint32_t* count) {
	const uint32_t face_count = lump->length / FACE_BINARY_SIZE;
	uint32_t size = face_count * sizeof(face);
	face* faces = malloc(face_count * sizeof(face));
	if (faces) {
		for (uint32_t i = 0; i < face_count; ++i) {
			memcpy(&(faces[i].iPlane), &lump->data[i * FACE_BINARY_SIZE], sizeof(uint16_t));
			memcpy(&faces[i].nPlaneSide, &lump->data[i * FACE_BINARY_SIZE + 2], sizeof(uint16_t));
			memcpy(&faces[i].iFirstEdge, &lump->data[i * FACE_BINARY_SIZE + 4], sizeof(uint32_t));
			memcpy(&faces[i].nEdges, &lump->data[i * FACE_BINARY_SIZE + 8], sizeof(uint16_t));
			memcpy(&faces[i].iTextureInfo, &lump->data[i * FACE_BINARY_SIZE + 10], sizeof(uint16_t));
			for (uint32_t j = 0; j < 4; ++j) {
				memcpy(&faces[i].nStyles[j], &lump->data[i * FACE_BINARY_SIZE + 12 + j], sizeof(uint8_t));
			}
			memcpy(&faces[i].nLightmapOffset, &lump->data[i * FACE_BINARY_SIZE + 16], sizeof(int32_t));
		}
		*count = face_count;
	}
	return faces;
}

float CalculatePointVecsProduct(const volatile float* point, const volatile float* vecs)
{
	volatile double val;
	volatile double tmp;

	val = (double)point[0] * (double)vecs[0]; // always do one operation at a time and save to memory
	tmp = (double)point[1] * (double)vecs[1];
	val = val + tmp;
	tmp = (double)point[2] * (double)vecs[2];
	val = val + tmp;
	val = val + (double)vecs[3];

	return (float)val;
}


void GetFaceExtents(face* f, texInfo* tex, vertex* verts, edge* edges, int32_t* surf_edges, int mins_out[2], int maxs_out[2])
{
	//CorrectFPUPrecision();

	float mins[2], maxs[2], val;
	int i, j, e;
	vertex* v;

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -999999;

	for (i = 0; i < f->nEdges; i++)
	{
		e = surf_edges[f->iFirstEdge + i];
		if (e >= 0)
		{
			v = &verts[edges[e].iVertex[0]];
		}
		else
		{
			v = &verts[edges[-e].iVertex[1]];
		}
		for (j = 0; j < 2; j++)
		{
			// The old code: val = v->point[0] * tex->vecs[j][0] + v->point[1] * tex->vecs[j][1] + v->point[2] * tex->vecs[j][2] + tex->vecs[j][3];
			//   was meant to be compiled for x86 under MSVC (prior to VS 11), so the intermediate values were stored as 64-bit double by default.
			// The new code will produce the same result as the old code, but it's portable for different platforms.
			// See this article for details: Intermediate Floating-Point Precision by Bruce-Dawson http://www.altdevblogaday.com/2012/03/22/intermediate-floating-point-precision/

			// The essential reason for having this ugly code is to get exactly the same value as the counterpart of game engine.
			// The counterpart of game engine is the function CalcFaceExtents in HLSDK.
			// So we must also know how Valve compiles HLSDK. I think Valve compiles HLSDK with VC6.0 in the past.
			vertex* axis = j == 0 ? tex->vecs[0] : tex->vecs[1];
			val = CalculatePointVecsProduct((vec_t*)v, (vec_t*)axis);

			if (val < mins[j])
			{
				mins[j] = val;
			}
			if (val > maxs[j])
			{
				maxs[j] = val;
			}
		}
	}

	for (i = 0; i < 2; i++)
	{
		mins_out[i] = (int)floor(mins[i] / TEXTURE_STEP);
		maxs_out[i] = (int)ceil(maxs[i] / TEXTURE_STEP);
	}
}

bool GetFaceLightmapSize(face* f, texInfo* tex, vertex* verts, edge* edges, int32_t* surf_edges, int size[2]) {
	int mins[2];
	int maxs[2];

	GetFaceExtents(f, tex, verts, edges, surf_edges, mins, maxs);

	size[0] = (maxs[0] - mins[0]);
	size[1] = (maxs[1] - mins[1]);

	bool badSurfaceExtents = false;
	if ((size[0] > MAX_SURFACE_EXTENT) || (size[1] > MAX_SURFACE_EXTENT) || size[0] < 0 || size[1] < 0)
	{
		printf("Bad surface extents (%d x %d)\n", size[0], size[1]);
		size[0] = min(size[0], MAX_SURFACE_EXTENT);
		size[1] = min(size[1], MAX_SURFACE_EXTENT);
		badSurfaceExtents = true;
	}

	size[0] += 1;
	size[1] += 1;

	return !badSurfaceExtents;
}

int32_t get_light_map_count(face* f)
{
	if (f->nLightmapOffset == UINT32_MAX) {
		return 0;
	}

	int32_t count = 0;
	for (int32_t i = 0; i < 4; ++i) {
		if (f->nStyles[i] != 0xFF) {
			++count;
		}
	}
	return count;
}


v2d calculate_lightmap_size(face* face, texInfo* tex_info, vertex* vertices, edge* edges, int32_t* surf_edges) {

	//float max_s = FLT_MIN;
	//float min_s = FLT_MAX;
	//float max_t = FLT_MIN;
	//float min_t = FLT_MAX;
	vertex* v;
	float mins[2], maxs[2], val;
	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;
	uint32_t texsize[2];

	for (int32_t i = 0; i < face->nEdges; i++)
	{
		int32_t e = surf_edges[face->iFirstEdge + i];
		if (e >= 0)
			v = vertices + edges[e].iVertex[0];
		else
			v = vertices + edges[-e].iVertex[1];

		for (int32_t j = 0; j < 2; j++)
		{
			val = v->point[0] * tex_info->vecs[j][0] +
				v->point[1] * tex_info->vecs[j][1] +
				v->point[2] * tex_info->vecs[j][2] +
				tex_info->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (int32_t i = 0; i < 2; i++)
	{
		//l->exactmins[i] = mins[i];
		//l->exactmaxs[i] = maxs[i];

		mins[i] = (float)floor(mins[i] / 16);
		maxs[i] = (float)ceil(maxs[i] / 16);

		//l->texmins[i] = (int)mins[i];
		texsize[i] = (int)(maxs[i] - mins[i] + 1);
		if (texsize[i] > 17) {
			printf("Bad surface extents\n");
			exit(EXIT_FAILURE);
		}
			//Error("Bad surface extents");
	}

	//texInfo* tex_info = &(tex_infos[face->iTextureInfo]);
	//printf("Number of edges: %d\n", face->nEdges);
	//printf("texture index: %d\n", tex_info->iMiptex);
	//for (uint32_t i = 0; i < face->nEdges; ++i) {
	//	//printf("First face edge: %d\n", face->iFirstEdge);
	//	int32_t edge_index = face_edges[face->iFirstEdge + i];
	//	//printf("Edge index: %d\n", edge_index);
	//	//printf("First face edge: %d\n", face_edges[0]);
	//	//uint32_t vertex_index = edge_index >= 0 ? edges[edge_index].iVertex[0] : edges[-edge_index].iVertex[1];
	//	//printf("Vertex index: %d\n", vertex_index);
	//	vertex* v = edge_index >= 0 ? &vertices[edges[edge_index].iVertex[0]] : &vertices[edges[-edge_index].iVertex[1]];
	//	//printf("Found vertex\n");

	//	//print_vertex(&v);
	//	//print_texinfo(tex_info);

	//	float s = v->x * tex_info->vS.x + v->y * tex_info->vS.y + v->z * tex_info->vS.z + tex_info->fSShift;
	//	float t = v->x * tex_info->vT.x + v->y * tex_info->vT.y + v->z * tex_info->vT.z + tex_info->fTShift;
	//	///printf("Calculated stuff\n");

	//	if (s < min_s) {
	//		min_s = s;
	//	}
	//	if (s > max_s) {
	//		max_s = s;
	//	}
	//	if (t < min_t) {
	//		min_t = t;
	//	}
	//	if (t > max_t) {
	//		max_t = t;
	//	}

	//}
	v2d size = { texsize[0], texsize[1] };

	//size.x = (int32_t)ceil(max_s / 16.0f) - (int32_t)floor(min_s / 16.0f) + 1;
	//size.y = (int32_t)ceil(max_t / 16.0f) - (int32_t)floor(min_t / 16.0f) + 1;
	return size;
}
