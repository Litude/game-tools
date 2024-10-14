#include <math.h>
#include <stdio.h>
#include "scaling.h"
#include "qdbmp/qdbmp.h"

static void scale_image_nn(color* source, color* dest, uint32_t width_source, uint32_t height_source, uint32_t width_dest, uint32_t height_dest) {
	for (uint32_t y = 0; y < height_dest; ++y) {
		for (uint32_t x = 0; x < width_dest; ++x) {
			uint32_t source_y = (uint32_t)(round((double)(y) / (double)(height_dest) * (double)(height_source)));
			uint32_t source_x = (uint32_t)(round((double)(x) / (double)(width_dest) * (double)(width_source)));
			dest[y * width_dest + x] = source[source_y * width_source + source_x];
		}
	}
}

static void scale_image_box(color* source, color* dest, uint32_t width_source, uint32_t height_source, uint32_t width_dest, uint32_t height_dest) {
	uint32_t box_width = round((double)width_source / width_dest);
	uint32_t box_height = round((double)height_source / height_dest);
	for (uint32_t y = 0; y < height_dest; ++y) {
		for (uint32_t x = 0; x < width_dest; ++x) {
			uint32_t r = 0;
			uint32_t g = 0;
			uint32_t b = 0;
			uint32_t samples = 0;

			uint32_t start_y = (uint32_t)(round((double)(y) / (double)(height_dest) * (double)(height_source)));
			uint32_t start_x = (uint32_t)(round((double)(x) / (double)(width_dest) * (double)(width_source)));

			for (uint32_t by = 0; by < box_height; ++by) {
				for (uint32_t bx = 0; bx < box_width; ++bx) {
					if (start_y + by < height_source && start_x + bx < width_source) {
						color sample = source[(start_x + bx) * height_source + (start_y + by)];
						//color sample = source[(start_y + by) * width_source + (start_x + bx)];
						r += sample.r;
						g += sample.g;
						b += sample.b;
						samples += 1;
					}
				}
			}
			r /= samples;
			g /= samples;
			b /= samples;
			color sampled = { r, g, b };
			dest[x * height_dest + y] = sampled;
			//dest[y * width_dest + x] = sampled;
		}
	}
}

void scale_image(uint32_t index, color* source, color* dest, uint32_t width_source, uint32_t height_source, uint32_t width_dest, uint32_t height_dest) {
	if (width_source == width_dest && height_source == height_dest) {
		printf("Not scaling light, same as original");
	}
	char source_name[255];
	char scaled_name[255];
	BMP* source_image = BMP_Create(width_source, height_source, 24);
	for (uint32_t y = 0; y < height_source; ++y) {
		for (uint32_t x = 0; x < width_source; ++x) {
			color* current = &source[y * width_source + x];
			BMP_SetPixelRGB(source_image, x, y, current->r, current->g, current->b);
		}
	}

	scale_image_nn(source, dest, width_source, height_source, width_dest, height_dest);

	BMP* scaled_image = BMP_Create(width_dest, height_dest, 24);
	for (uint32_t y = 0; y < height_dest; ++y) {
		for (uint32_t x = 0; x < width_dest; ++x) {
			color* current = &dest[y * width_dest + x];
			BMP_SetPixelRGB(scaled_image, x, y, current->r, current->g, current->b);
		}
	}

	sprintf(source_name, "%d_original.bmp", index);
	sprintf(scaled_name, "%d_scaled.bmp", index);
	BMP_WriteFile(source_image, source_name);
	BMP_WriteFile(scaled_image, scaled_name);
	BMP_Free(source_image);
	BMP_Free(scaled_image);
}
