#include <iostream>
#include "BspFile.hpp"

BspLump* BspFile::getLump(LumpType type) {
	return &this->lumps[type];
}

BspFile* BspFile::readFromFile(std::ifstream &filestream) {
	BspFile* fileData = new BspFile;

	filestream.read(reinterpret_cast<char *>(&fileData->version), sizeof(int32_t));
	if (fileData->version != 30) {
		std::cerr << "Invalid BSP version or not a BSP file.\n";
		delete fileData;
		return nullptr;
	}

	for (uint32_t i = 0; i < NUM_LUMPS; ++i) {
		filestream.read(reinterpret_cast<char *>(&fileData->lumps[i].offset), sizeof(int32_t));
		filestream.read(reinterpret_cast<char *>(&fileData->lumps[i].length), sizeof(int32_t));
	}

	for (uint32_t i = 0; i < NUM_LUMPS; ++i) {
		BspLump *currentLump = &fileData->lumps[i];
		currentLump->data = std::unique_ptr<char []>(new char[currentLump->length]);

		filestream.seekg(currentLump->offset);
		filestream.read(currentLump->data.get(), currentLump->length);
		if (!filestream.good()) {
			std::cerr << "Unexpected end of lump in BSP file.\n";
			delete fileData;
			return nullptr;
		}
		currentLump->offset = 0;
	}

	return fileData;
}

bool BspFile::writeLump(LumpType type, std::ofstream &filestream) {
	constexpr char padding = 0;
	BspLump* currentLump = &this->lumps[type];
	uint8_t padding_amount = (currentLump->length % 4) ? 4 - currentLump->length % 4 : 0;
	currentLump->offset = static_cast<int32_t>(filestream.tellp());
	filestream.write(currentLump->data.get(), currentLump->length);
	filestream.write(&padding, padding_amount);

	return filestream.good();
}

bool BspFile::writeHeader(std::ofstream &filestream) {
	filestream.seekp(0);
	filestream.write(reinterpret_cast<const char *>(&this->version), sizeof(int32_t));
	for (uint32_t i = 0; i < NUM_LUMPS; ++i) {
		filestream.write(reinterpret_cast<const char*>(&this->lumps[i].offset), sizeof(int32_t));
		filestream.write(reinterpret_cast<const char*>(&this->lumps[i].length), sizeof(int32_t));
	}
	return filestream.good();
}

bool BspFile::writeToFile(std::ofstream &filestream) {
	return (
		this->writeHeader(filestream) &&
		this->writeLump(PLANES, filestream) &&
		this->writeLump(LEAVES, filestream) &&
		this->writeLump(VERTICES, filestream) &&
		this->writeLump(NODES, filestream) &&
		this->writeLump(TEXINFO, filestream) &&
		this->writeLump(FACES, filestream) &&
		this->writeLump(CLIPNODES, filestream) &&
		this->writeLump(MARKSURFACES, filestream) &&
		this->writeLump(SURFEDGES, filestream) &&
		this->writeLump(EDGES, filestream) &&
		this->writeLump(MODELS, filestream) &&
		this->writeLump(LIGHTING, filestream) &&
		this->writeLump(VISIBILITY, filestream) &&
		this->writeLump(ENTITIES, filestream) &&
		this->writeLump(TEXTURES, filestream) &&
		this->writeHeader(filestream));
}

void BspFile::replaceLump(LumpType type, BspLump* newLump) {
	this->lumps[type] = std::move(*newLump);
}
