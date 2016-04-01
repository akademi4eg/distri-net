#include <iostream>
#include <fstream>
#include "CLocalFileReader.h"

#define Log(x) (std::cout << x << std::endl)

std::unique_ptr<DataEntry> CLocalFileReader::loadData(const SDataKey& key)
{
	std::streampos fsize, finitpos;
	std::ifstream input(key.getFilename().c_str(), std::ios::in | std::ios::binary);
	if (!input)
	{
		Log("Failed to create input stream!");
		return std::unique_ptr<DataEntry>(nullptr);
	}
	finitpos = input.tellg();
    input.seekg( 0, std::ios::end );
    fsize = input.tellg() - finitpos;
    input.seekg(finitpos, std::ios::beg);
    input.clear();
	DataEntry *result = new DataEntry(fsize/sizeof(DataEntry::value_type));
	input.read(reinterpret_cast<char*>(&(*result)[0]), fsize);
	if (!input)
	{
		Log("Failed to read from stream!");
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
	{
		Log("Failed to create output stream!");
		return false;
	}
	output.write(reinterpret_cast<const char*>(&data[0]), data.size()*sizeof(DataEntry::value_type));
	if (!output)
	{
		Log("Failed to write to stream!");
		output.close();
		return false;
	}
	output.close();
	return true;
}
