#include "IDataReader.h"

class CLocalFileReader : public IDataReader
{
public:
	CLocalFileReader() {};
	~CLocalFileReader() {};

	bool saveData(const SDataKey& key, const DataEntry& data);
	std::unique_ptr<DataEntry> loadData(const SDataKey& key);
};
