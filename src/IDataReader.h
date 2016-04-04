#pragma once

#include <memory>
#include <vector>

typedef std::vector<double> DataEntry;

struct SDataKey
{
	SDataKey(const std::string& fname = std::string(), uintmax_t idx = 0) : sSource(fname), iIndex(idx) {};
	std::string toString() const {
		return sSource + "\n" + std::to_string(iIndex);
	};
	std::string toPrettyString() const {
		return sSource + "[index = " + std::to_string(iIndex) + "]";
	};
	std::string getFilename() const {return sSource + ":" + std::to_string(iIndex);}

	std::string sSource;
	uintmax_t iIndex;
};

class IDataReader
{
public:
	virtual bool saveData(const SDataKey& key, const DataEntry& data) = 0;
	virtual std::unique_ptr<DataEntry> loadData(const SDataKey& key) = 0;

	virtual ~IDataReader() {};
};
