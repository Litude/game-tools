#include <sstream>
#include <iostream>
#include <algorithm>
#include "Entity.hpp"

std::vector<Entity> Entity::createEntriesFromLumpData(BspLump* lump) {
	std::vector<Entity> entities;
	entities.reserve(MAX_MAP_ENTITIES);

	std::istringstream stream(lump->data.get());
	std::string line;
	char key[MAX_ENTITY_KEY];
	char value[MAX_ENTITY_VALUE];
	Entity* current = nullptr;
	while (std::getline(stream, line, static_cast<char>(0x0A))) {
		if (line == "{") {
			current = new Entity();
		}
		else if (line == "}") {
			if (current) {
				entities.push_back(*current);
			}
		}
		else {
			if (current) {
				if (std::sscanf(line.c_str(), "\"%[^\"]\" \"%[^\"]\"", key, value) == 2) {
					current->entries.push_back(std::make_pair(std::string(key), std::string(value)));
				}
			}
		}
	}
	return entities;
}

BspLump* Entity::createDataLumpFromEntries(std::vector<Entity>& entities) {
	std::stringstream ss;
	for (auto& entity : entities) {
		ss << "{\n";
		for (auto& values : entity.entries) {
			ss << '"' << values.first << "\" \"" << values.second << "\"\n";
		}
		ss << "}\n";
	}
	const std::string string = ss.str();
	BspLump* bspLump = new BspLump;
	bspLump->length = string.length() + 1;
	bspLump->data = std::unique_ptr<char[]>(new char[bspLump->length]);
	std::memcpy(bspLump->data.get(), string.c_str(), bspLump->length);
	return bspLump;
}

bool Entity::updateWithRemovedModel(std::vector<Entity>& entities, uint32_t modelNumber) {
	uint32_t model = 0;
	auto it = std::find_if(entities.begin(), entities.end(), [&](Entity entity) {
		return std::find_if(entity.entries.begin(), entity.entries.end(), [&](std::pair<std::string, std::string> &entry) {
			if (entry.first == "model" && std::sscanf(entry.second.c_str(), "*%d", &model)) {
				return model == modelNumber;
			}
			else {
				return false;
			}
		}) != entity.entries.end();
	});
	if (it != entities.end()) {
		uint32_t index = it - entities.begin();
		entities.erase(it);
		for (uint32_t i = index; i < entities.size(); ++i) {
			std::for_each(entities[i].entries.begin(), entities[i].entries.end(), [&](std::pair<std::string, std::string> &entry) {
				if (entry.first == "model" && std::sscanf(entry.second.c_str(), "*%d", &model) && model > modelNumber) {
					entry.second = "*" + std::to_string(model - 1);
				}
			});
		}
		return true;
	}
	else {
		return false;
	}
}
