#ifndef ENTITY_HPP
#define ENTITY_HPP


#include <map>
#include <string>
#include <vector>
#include "BspFile.hpp"

constexpr int32_t MAX_MAP_ENTITIES = 1024;
constexpr int32_t MAX_ENTITY_KEY = 32;
constexpr int32_t MAX_ENTITY_VALUE = 1024;

class Entity {
public:
	static std::vector<Entity> createEntriesFromLumpData(BspLump* lump);
	static BspLump* createDataLumpFromEntries(std::vector<Entity>& entities);
	static bool updateWithRemovedModel(std::vector<Entity>& entities, uint32_t modelNumber);
private:
	std::vector<std::pair<std::string, std::string>> entries;
};
#endif // !ENTITY_HPP
