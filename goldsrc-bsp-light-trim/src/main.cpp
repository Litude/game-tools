#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include "Face.hpp"
#include "Lightning.hpp"
#include "BspFile.hpp"

//constexpr uint32_t NUM_LIGHTS = 524288;
constexpr uint32_t NUM_LIGHTS = 131072;
constexpr uint8_t NO_LIGHT_STYLES[] = { 0xFF, 0xFF, 0xFF, 0xFF };

int main (int argc, char *argv[])
{
	std::vector<std::string> args(argv, argv + argc);
	std::cout << "BSPLightTrim v1.0\n";
	if (args.size() < 2) {
		std::cout << "Trims excess light entries from a BSP file.\n";
		std::cout << "Usage: " << args[0] << " <bspfile>\n";
		return EXIT_FAILURE;
	}

	const std::filesystem::path inputName = std::filesystem::path(args[1]);

	std::ifstream inputFile(inputName, std::ios::binary);
	if (!inputFile.is_open()) {
		std::cerr << "Failed opening file, error occured\n";
		return EXIT_FAILURE;
	}

	std::cout << "Reading BSP file " << inputName << " ...\n";
	std::unique_ptr<BspFile> input = std::unique_ptr<BspFile> (BspFile::readFromFile(inputFile));
	inputFile.close();

	std::vector<Face> faces = Face::createEntriesFromLumpData(input->getLump(LumpType::FACES));
	const std::vector<Lightning> lightning = Lightning::createEntriesFromLumpData(input->getLump(LumpType::LIGHTING));

	//const int32_t numFaces = Face::numberOfEntriesInData(input->getLump(LumpType::FACES));
	for (uint32_t i = 0; i < faces.size(); ++i) {
		faces[i].updateLightmapData(lightning);
	}

	//TODO: Better trim
	const std::vector<Lightning> trimmedLightning = std::vector<Lightning>(lightning.begin(), lightning.begin() + NUM_LIGHTS);

	for (uint32_t i = 0; i < faces.size(); ++i) {
		if (faces[i].getLightmapOffset() == 0xFFFFFFFF) {
			std::cout << "found one" << std::endl;
		}
		if (i > 200) {
			faces[i].setLightmapOffset(0xFFFFFFFF);
			faces[i].setLightStyles(NO_LIGHT_STYLES);
		}
		//if (face.getLightmapOffset() != 0xFFFFFFFF) {
		//	uint32_t smallestDistance = UINT32_MAX;
		//	int32_t smallestIndex = -1;
		//	for (uint32_t i = 0; i < trimmedLightning.size(); ++i) {
		//		const uint32_t distance = face.getLightmap().differenceTo(trimmedLightning[i]);
		//		if (distance < smallestDistance) {
		//			smallestDistance = distance;
		//			smallestIndex = i;
		//		}
		//	}
		//	if (smallestIndex != -1) {
		//		face.setLightmap(trimmedLightning[smallestIndex]);
		//		face.setLightmapIndex(smallestIndex);
		//	}
		//	else {
		//		std::cerr << "Couldn't find a close color match for face" << std::endl;
		//	}
		//}
	}

	auto trimmedLightningLump = Lightning::createDataLumpFromEntries(trimmedLightning);
	auto fixedFaceLump = Face::createDataLumpFromEntries(faces);

	input->replaceLump(LumpType::LIGHTING, trimmedLightningLump);
	input->replaceLump(LumpType::FACES, fixedFaceLump);

	std::filesystem::path outputName = inputName;
	outputName.replace_filename(outputName.filename().stem().string() + "_trimmed" + inputName.extension().string());

	std::cout << "Writing BSP file " << outputName << " ...\n";

	std::ofstream outputFile(outputName, std::ios::binary);
	input->writeToFile(outputFile);
	outputFile.close();

	return EXIT_SUCCESS;
}
