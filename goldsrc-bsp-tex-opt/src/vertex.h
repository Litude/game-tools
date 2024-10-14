#ifndef VERTEX_H
#define VERTEX_H

#include "vector.h"

#define VERTEX_BINARY_SIZE 12

//typedef v3df vertex;

typedef struct
{
	float	point[3];
} vertex;

vertex* create_vertices_from_data(bspLump* vertices_lump, uint32_t* count);
void print_vertex(vertex* vertex);

#endif // !VERTEX_H
