#include "BspFile.hpp"
#include "Lightning.hpp"

uint32_t Lightning::differenceTo(Lightning const& that) {
	const int diffR = this->r - that.r;
	const int diffG = this->g - that.g;
	const int diffB = this->b - that.b;

	return diffR * diffR + diffG * diffG + diffB * diffB;
}

std::vector<Lightning> Lightning::createEntriesFromLumpData(BspLump* lump) {
	const int32_t numLights = numberOfEntriesInData(lump);
	std::vector<Lightning> lights(numLights);

	for (size_t i = 0; i < numLights; ++i) {
		lights[i].r = lump->data[i * LIGHTNING_SIZE];
		lights[i].g = lump->data[i * LIGHTNING_SIZE + 1];
		lights[i].b = lump->data[i * LIGHTNING_SIZE + 2];
	}
	return lights;
}

int32_t Lightning::numberOfEntriesInData(BspLump* data)
{
	return data->length / LIGHTNING_SIZE;
}

BspLump* Lightning::createDataLumpFromEntries(std::vector<Lightning> const& entries) {
	BspLump* bspLump = new BspLump;
	bspLump->length = static_cast<int32_t>(entries.size()) * LIGHTNING_SIZE;
	bspLump->data = std::unique_ptr<char[]>(new char[entries.size() * LIGHTNING_SIZE]);
	char* curPtr = bspLump->data.get();
	for (auto& light : entries) {
		light.writeToBuffer(curPtr);
		curPtr += LIGHTNING_SIZE;
	}
	return bspLump;
}

void Lightning::writeToBuffer(char* const data) const {
	data[0] = this->r;
	data[1] = this->g;
	data[2] = this->b;
}
