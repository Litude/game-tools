#include <iostream>
#include <cstring>
#include "Face.hpp"

std::vector<Face> Face::createEntriesFromLumpData(BspLump* lump)
{
	const int32_t numFaces = numberOfEntriesInData(lump);
	std::vector<Face> faces(numFaces);
	for (size_t i = 0; i < numFaces; ++i) {
		faces[i].iPlane = *reinterpret_cast<uint16_t*>(&lump->data[i * FACE_SIZE]);
		faces[i].nPlaneSide = *reinterpret_cast<uint16_t*>(&lump->data[i * FACE_SIZE + 2]);
		faces[i].iFirstEdge = *reinterpret_cast<uint32_t*>(&lump->data[i * FACE_SIZE + 4]);
		faces[i].nEdges = *reinterpret_cast<uint16_t*>(&lump->data[i * FACE_SIZE + 8]);
		faces[i].iTextureInfo = *reinterpret_cast<uint16_t*>(&lump->data[i * FACE_SIZE + 10]);
		for (int32_t j = 0; j < NUM_LIGHT_STYLES; ++j) {
			faces[i].nStyles[j] = *reinterpret_cast<uint8_t*>(&lump->data[i * FACE_SIZE + 12 + j]);
		}
		faces[i].nLightmapOffset = *reinterpret_cast<uint32_t*>(&lump->data[i * FACE_SIZE + 16]);
	}
	return faces;
}

void Face::updateLightmapData(const std::vector<Lightning>& lightmap) {
	if (this->nLightmapOffset != 0xFFFFFFFF) {
		this->dLightmap = lightmap[this->nLightmapOffset / 3];
	}
}

void Face::setLightStyles(const uint8_t newStyles[4]) {
	for (uint32_t i = 0; i < 4; ++i) {
		this->nStyles[i] = newStyles[i];
	}
}

BspLump* Face::createDataLumpFromEntries(std::vector<Face>& entries) {
	BspLump* bspLump = new BspLump;
	bspLump->length = static_cast<int32_t>(entries.size()) * FACE_SIZE;
	bspLump->data = std::unique_ptr<char[]>(new char[entries.size() * FACE_SIZE]);
	char* curPtr = bspLump->data.get();
	for (auto& face : entries) {
		face.writeToBuffer(curPtr);
		curPtr += FACE_SIZE;
	}
	return bspLump;
}

/* static */ int32_t Face::numberOfEntriesInData(BspLump* data)
{
	return data->length / FACE_SIZE;
}

void Face::writeToBuffer(char* data) const {
	std::memcpy(data, &this->iPlane, sizeof(uint16_t));
	std::memcpy(data + 2, &this->nPlaneSide, sizeof(uint16_t));
	std::memcpy(data + 4, &this->iFirstEdge, sizeof(uint32_t));
	std::memcpy(data + 8, &this->nEdges, sizeof(uint16_t));
	std::memcpy(data + 10, &this->iTextureInfo, sizeof(uint16_t));
	for (int32_t i = 0; i < NUM_LIGHT_STYLES; ++i) {
		std::memcpy(data + 12 + i, &this->nStyles[i], sizeof(uint8_t));
	}
	std::memcpy(data + 16, &this->nLightmapOffset, sizeof(uint32_t));
}
