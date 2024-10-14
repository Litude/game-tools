#ifndef LIGHTNING_HPP
#define LIGHTNING_HPP

#include <cstdint>
#include <vector>

constexpr int32_t LIGHTNING_SIZE = 3;

class Lightning {
public:
	void writeToBuffer(char* const data) const;
	uint32_t differenceTo(Lightning const& that);
	static std::vector<Lightning> createEntriesFromLumpData(BspLump* lump);
	static int numberOfEntriesInData(BspLump* data);
	static BspLump* createDataLumpFromEntries(std::vector<Lightning> const& entries);
private:
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

#endif // !LIGHTNING_HPP
