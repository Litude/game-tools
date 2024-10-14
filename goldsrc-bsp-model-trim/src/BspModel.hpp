#ifndef MODEL_HPP
#define MODEL_HPP

#include "BspFile.hpp"
#include <cstdint>

constexpr int32_t BSPMODEL_SIZE = 64;

typedef struct _vector3d
{
	float x, y, z;
} vector3d;

#define MAX_MAP_HULLS 4

class BspModel {
public:
	static std::vector<BspModel> createEntriesFromLumpData(BspLump* lump);
	static int numberOfEntriesInData(BspLump* data);
	static BspLump* createDataLumpFromEntries(std::vector<BspModel> const& entries);
	void writeToBuffer(char* const data) const;
private:
	float nMins[3], nMaxs[3];          // Defines bounding box
	vector3d vOrigin;                  // Coordinates to move the // coordinate system
	int32_t iHeadnodes[MAX_MAP_HULLS]; // Index into nodes array
	int32_t nVisLeafs;                 // ???
	int32_t iFirstFace, nFaces;        // Index and count into faces
};

#endif // !MODEL_HPP
