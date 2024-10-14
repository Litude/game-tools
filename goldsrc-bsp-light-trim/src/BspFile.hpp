#ifndef BSPFILE_HPP
#define BSPFILE_HPP

/*
This code is mostly based on information available at The hlbsp Project:
http://hlbsp.sourceforge.net/index.php?content=bspdef
*/
#include <cstdint>
#include <fstream>

enum LumpType {
	ENTITIES,
	PLANES,
	TEXTURES,
	VERTICES,
	VISIBILITY,
	NODES,
	TEXINFO,
	FACES,
	LIGHTING,
	CLIPNODES,
	LEAVES,
	MARKSURFACES,
	EDGES,
	SURFEDGES,
	MODELS,
	NUM_LUMPS
};

struct BspLump {
	int32_t offset = 0; // File offset to data
	int32_t length = 0; // Length of data
	std::unique_ptr<char []> data; // Data
};

class BspFile {
public:
	static BspFile* readFromFile(std::ifstream &filestream);
	bool writeToFile(std::ofstream &filestream);
	BspLump* getLump(LumpType type);
	void replaceLump(LumpType type, BspLump* newLump);
private:
	bool writeHeader(std::ofstream &filestream);
	bool writeLump(LumpType type, std::ofstream &filestream);
	int32_t version = -1;           // Must be 30 for a valid HL BSP file
	BspLump lumps[NUM_LUMPS]; // Stores the directory of lumps
};

#endif //!BSPFILE_HPP
