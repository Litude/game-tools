#ifndef FACE_HPP
#define FACE_HPP

#include <cstdint>
#include <vector>
#include "BspFile.hpp"
#include "Lightning.hpp"

constexpr int32_t FACE_SIZE = 20;
constexpr int32_t NUM_LIGHT_STYLES = 4;

class Face
{
public:
	void writeToBuffer(char* data) const;
	void updateLightmapData(const std::vector<Lightning>& lightmap);
	void setLightmap(const Lightning& newLight) { dLightmap = newLight; };
	void setLightmapIndex(const uint32_t newIndex) { nLightmapOffset = newIndex * LIGHTNING_SIZE; }
	void setLightmapOffset(const uint32_t newOffset) { nLightmapOffset = newOffset; }
	void setLightStyles(const uint8_t newStyles[4]);
	uint32_t getLightmapOffset() { return nLightmapOffset; }
	Lightning getLightmap() { return dLightmap; }
	static BspLump* createDataLumpFromEntries(std::vector<Face>& entries);
	static std::vector<Face> createEntriesFromLumpData(BspLump* data);
	static int numberOfEntriesInData(BspLump* data);
private:
	uint16_t iPlane;          // Plane the face is parallel to
	uint16_t nPlaneSide;      // Set if different normals orientation
	uint32_t iFirstEdge;      // Index of the first surfedge
	uint16_t nEdges;          // Number of consecutive surfedges
	uint16_t iTextureInfo;    // Index of the texture info structure
	uint8_t nStyles[NUM_LIGHT_STYLES];       // Specify lighting styles
	uint32_t nLightmapOffset; // Offsets into the raw lightmap data
	Lightning dLightmap;     // Actual lightmap data
};

#endif !FACE_HPP
