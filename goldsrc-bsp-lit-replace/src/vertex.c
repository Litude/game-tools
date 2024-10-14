#include <stdlib.h>
#include <string.h>
#include "bspfile.h"
#include "vertex.h"

vertex* create_vertices_from_data(bspLump* vertices_lump, uint32_t* count) {
	uint32_t num_vertices = vertices_lump->length / VERTEX_BINARY_SIZE;
	vertex* vertices = malloc(num_vertices * sizeof(vertex));
	if (vertices) {
		for (uint32_t i = 0; i < num_vertices; ++i) {
			//Lazy, should probably copy each field individually
			memcpy(&vertices[i].point[0], &vertices_lump->data[i * VERTEX_BINARY_SIZE], sizeof(float));
			memcpy(&vertices[i].point[1], &vertices_lump->data[i * VERTEX_BINARY_SIZE + 4], sizeof(float));
			memcpy(&vertices[i].point[2], &vertices_lump->data[i * VERTEX_BINARY_SIZE + 8], sizeof(float));
		}
		*count = num_vertices;
	}
	return vertices;
}

void print_vertex(vertex* vertex) {
	printf("Vertex:\nx %f\ny %f\nz %f\n", vertex->point[0], vertex->point[1], vertex->point[2]);
}