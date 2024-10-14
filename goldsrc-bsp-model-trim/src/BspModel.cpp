#include <vector>
#include "BspModel.hpp"
std::vector<BspModel> BspModel::createEntriesFromLumpData(BspLump* lump) {
	const int32_t numModels = numberOfEntriesInData(lump);
	std::vector<BspModel> models(numModels);

	for (size_t i = 0; i < numModels; ++i) {
		models[i].nMins[0] = *reinterpret_cast<float*>(&lump->data[i * BSPMODEL_SIZE]);
		models[i].nMins[1] = *reinterpret_cast<float*>(&lump->data[i * BSPMODEL_SIZE + 4]);
		models[i].nMins[2] = *reinterpret_cast<float*>(&lump->data[i * BSPMODEL_SIZE + 8]);
		models[i].nMaxs[0] = *reinterpret_cast<float*>(&lump->data[i * BSPMODEL_SIZE + 12]);
		models[i].nMaxs[1] = *reinterpret_cast<float*>(&lump->data[i * BSPMODEL_SIZE + 16]);
		models[i].nMaxs[2] = *reinterpret_cast<float*>(&lump->data[i * BSPMODEL_SIZE + 20]);
		models[i].vOrigin.x = *reinterpret_cast<float*>(&lump->data[i * BSPMODEL_SIZE + 24]);
		models[i].vOrigin.y = *reinterpret_cast<float*>(&lump->data[i * BSPMODEL_SIZE + 28]);
		models[i].vOrigin.z = *reinterpret_cast<float*>(&lump->data[i * BSPMODEL_SIZE + 32]);
		
		for (uint32_t j = 0; j < MAX_MAP_HULLS; ++j) {
			models[i].iHeadnodes[j] = *reinterpret_cast<int32_t*>(&lump->data[i * BSPMODEL_SIZE + 36 + j * sizeof(int32_t)]);
		}

		models[i].nVisLeafs = *reinterpret_cast<int32_t*>(&lump->data[i * BSPMODEL_SIZE + 52]);
		models[i].iFirstFace = *reinterpret_cast<int32_t*>(&lump->data[i * BSPMODEL_SIZE + 56]);
		models[i].nFaces = *reinterpret_cast<int32_t*>(&lump->data[i * BSPMODEL_SIZE + 60]);
	}
	return models;
}

int32_t BspModel::numberOfEntriesInData(BspLump* data)
{
	return data->length / BSPMODEL_SIZE;
}

BspLump* BspModel::createDataLumpFromEntries(std::vector<BspModel> const& entries) {
	BspLump* bspLump = new BspLump;
	bspLump->length = static_cast<int32_t>(entries.size()) * BSPMODEL_SIZE;
	bspLump->data = std::unique_ptr<char[]>(new char[entries.size() * BSPMODEL_SIZE]);
	char* curPtr = bspLump->data.get();
	for (auto& model : entries) {
		model.writeToBuffer(curPtr);
		curPtr += BSPMODEL_SIZE;
	}
	return bspLump;
}

void BspModel::writeToBuffer(char* const data) const {
	std::memcpy(data, &this->nMins[0], sizeof(float));
	std::memcpy(data + 4, &this->nMins[1], sizeof(float));
	std::memcpy(data + 8, &this->nMins[2], sizeof(float));
	std::memcpy(data + 12, &this->nMaxs[0], sizeof(float));
	std::memcpy(data + 16, &this->nMaxs[1], sizeof(float));
	std::memcpy(data + 20, &this->nMaxs[2], sizeof(float));
	std::memcpy(data + 24, &this->vOrigin.x, sizeof(float));
	std::memcpy(data + 28, &this->vOrigin.y, sizeof(float));
	std::memcpy(data + 32, &this->vOrigin.z, sizeof(float));

	for (uint32_t j = 0; j < MAX_MAP_HULLS; ++j) {
		std::memcpy(data + 36 + j * sizeof(int32_t), &this->iHeadnodes[j], sizeof(int32_t));
	}

	std::memcpy(data + 52, &this->nVisLeafs, sizeof(int32_t));
	std::memcpy(data + 56, &this->iFirstFace, sizeof(int32_t));
	std::memcpy(data + 60, &this->nFaces, sizeof(int32_t));
}
