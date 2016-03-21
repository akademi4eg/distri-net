#include <fstream>
#include "CLocalFileReader.h"

#define Log(x) (std::cout << x << std::endl)

std::unique_ptr<DataEntry> CLocalFileReader::loadData(const SDataKey& key)
{
	std::ifstream input(key.getFilename().c_str(), std::ios::in | std::ios::binary);
	if (!input)
		return std::unique_ptr<DataEntry>(nullptr);
	DataEntry *result = new DataEntry(key.iEntrySize);
	input.read(reinterpret_cast<char*>(&(*result)[0]), key.iEntrySize*sizeof(DataEntry::value_type));
	if (!input)
	{
		delete result;
		result = nullptr;
	}
	input.close();
	return std::unique_ptr<DataEntry>(result);
}

bool CLocalFileReader::saveData(const SDataKey& key, const DataEntry& data)
{
	std::ofstream output(key.getFilename().c_str(), std::ios::out | std::ios::binary);
	if (!output)
		return false;
	output.write(reinterpret_cast<const char*>(&data[0]), data.size()*sizeof(DataEntry::value_type));
	if (!output)
	{
		output.close();
		return false;
	}
	output.close();
	return true;
}
