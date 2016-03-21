#pragma once

#include <memory>
#include <vector>

typedef std::vector<double> DataEntry;

struct SDataKey
{
	std::string toString() const {
		return sSource + "\n" + std::to_string(iIndex) + "\n" + std::to_string(iEntrySize);
	};
	std::string getFilename() const {return sSource + ":" + std::to_string(iIndex);}

	std::string sSource;
	uintmax_t iIndex;
	size_t iEntrySize;
};

class IDataReader
{
public:
	virtual bool saveData(const SDataKey& key, const DataEntry& data) = 0;
	virtual std::unique_ptr<DataEntry> loadData(const SDataKey& key) = 0;

	virtual ~IDataReader() {};
};
