#ifndef VECTOR_H
#define VECTOR_H

typedef struct
{
	float x, y, z;
} vec3;

typedef struct
{
	int32_t x, y;
} v2d;

typedef struct
{
	float x, y, z;
} v3df;

inline float CalculatePointVecsProduct(const volatile float* point, const volatile float* vecs)
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

#endif
