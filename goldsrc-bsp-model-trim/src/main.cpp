#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include "Face.hpp"
#include "Lightning.hpp"
#include "BspFile.hpp"
#include "BspModel.hpp"
#include "Entity.hpp"

int main (int argc, char *argv[])
{
	std::vector<std::string> args(argv, argv + argc);
	std::cout << "BSPModelTrim v1.0\n";
	if (args.size() < 3) {
		std::cout << "Trims world models from a BSP file.\n";
		std::cout << "Usage: " << args[0] << " <bspfile> <modelnumber1> [modelnumber2] [modelnumber3] ...\n";
		return EXIT_FAILURE;
	}

	//const std::filesystem::path inputName = std::filesystem::path("de_alcatraz.bsp");
	const std::filesystem::path inputName = std::filesystem::path(args[1]);

	std::vector<uint32_t> modelNumbers(args.size() - 2);
	for (uint32_t i = 0; i < modelNumbers.size(); ++i) {
		modelNumbers[i] = std::stoi(args[2 + i]);
	}

	std::ifstream inputFile(inputName, std::ios::binary);
	if (!inputFile.is_open()) {
		std::cerr << "Failed opening file, error occured\n";
		return EXIT_FAILURE;
	}

	std::cout << "Reading BSP file " << inputName << " ...\n";
	std::unique_ptr<BspFile> input = std::unique_ptr<BspFile> (BspFile::readFromFile(inputFile));
	inputFile.close();

	std::vector<BspModel> models = BspModel::createEntriesFromLumpData(input->getLump(LumpType::MODELS));
	std::vector<Entity> entities = Entity::createEntriesFromLumpData(input->getLump(LumpType::ENTITIES));

	for (uint32_t i = 0; i < modelNumbers.size(); ++i) {
		models.erase(models.begin() + modelNumbers[i]);
		Entity::updateWithRemovedModel(entities, modelNumbers[i]);

		for (uint32_t j = i + 1; j < modelNumbers.size(); ++j) {
			if (modelNumbers[j] > modelNumbers[i]) {
				modelNumbers[j] -= 1;
			}
		}
	}

	auto trimmedModelsLump = BspModel::createDataLumpFromEntries(models);
	auto trimmedEntitiesLump = Entity::createDataLumpFromEntries(entities);

	input->replaceLump(LumpType::MODELS, trimmedModelsLump);
	input->replaceLump(LumpType::ENTITIES, trimmedEntitiesLump);

	std::filesystem::path outputName = inputName;
	outputName.replace_filename(outputName.filename().stem().string() + "_trimmed" + inputName.extension().string());

	std::cout << "Writing BSP file " << outputName << " ...\n";

	std::ofstream outputFile(outputName, std::ios::binary);
	input->writeToFile(outputFile);
	outputFile.close();

	return EXIT_SUCCESS;
}
