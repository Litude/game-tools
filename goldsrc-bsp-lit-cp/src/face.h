#ifndef FACE_H
#define FACE_H

#include <stdint.h>
#include <stdbool.h>
#include "edge.h"
#include "texinfo.h"
#include "vector.h"
#include "vertex.h"

#define FACE_BINARY_SIZE 20

typedef struct
{
	uint16_t iPlane;          // Plane the face is parallel to
	uint16_t nPlaneSide;      // Set if different normals orientation
	uint32_t iFirstEdge;      // Index of the first surfedge
	uint16_t nEdges;          // Number of consecutive surfedges
	uint16_t iTextureInfo;    // Index of the texture info structure
	uint8_t nStyles[4];       // Specify lighting styles
	uint32_t nLightmapOffset; // Offsets into the raw lightmap data
} face;

face* create_faces_from_data(bspLump* lump, uint32_t* count);
v2d calculate_lightmap_size(face* face, texInfo* tex_infos, vertex* vertices, edge* edges, int32_t* face_edges);
bool GetFaceLightmapSize(face* f, texInfo* tex, vertex* verts, edge* edges, int32_t* surf_edges, int size[2]);
int32_t get_light_map_count(face* f);

#endif // !FACE_H
