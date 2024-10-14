#include <stdint.h>
#include "vector.h"

typedef struct _BSPPLANE
{
    vec3 vNormal; // The planes normal vector
    float fDist;      // Plane equation is: vNormal * X = fDist
    int32_t nType;    // Plane type, see #defines
} BSPPLANE;
