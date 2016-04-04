#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include "CTasksCreator.h"
#include "../RequestsFactory.h"
#include "../Version.h"
#define Log(x) (std::cout << x << std::endl)

int main(int argc, const char* argv[])
{
	Log("Manager version "+c_sProductVersion);
	if (argc < 2)
	{
		Log("Input file expected.");
		return 1;
	}
	CTasksCreator manager("localhost", 5672, AMQP::Login("dnet", "111111"),
			"/dnet");
	std::ifstream infile(argv[1]);
	if (!infile)
	{
		Log("Failed to open file " + std::string(argv[1]));
		return 2;
	}
	
	std::string line;
	std::map<std::string, SDataKey> vars;
	while (std::getline(infile, line))
	{
		if (line.substr(0, 2) == "//") // comment
			continue;
		std::istringstream iss(line);
		std::string cmd;
		iss >> cmd;
		Log(cmd);
	}
	
	infile.close();
	manager.run();
	return 0;
}
