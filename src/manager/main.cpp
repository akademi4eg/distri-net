#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include "CTasksCreator.h"
#include "../RequestsFactory.h"
#include "../Version.h"
#define Log(x) (std::cout << x << std::endl)

void applyWordTrim(std::string& cmd)
{
	cmd.erase(cmd.find_last_not_of(" \t")+1);
	size_t firstPos = cmd.find_first_not_of(" \t");
	if (firstPos != std::string::npos)
		cmd = cmd.substr(firstPos);
}

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
		std::istringstream issCmd(line);
		std::string cmd;
		issCmd >> cmd;
		size_t iSkip = cmd.size();
		applyWordTrim(cmd);
		if (cmd.empty()) // blank line
			continue;
		std::string args = issCmd.str().substr(iSkip);
		std::istringstream issArgs = std::istringstream(args);
		std::string curArg;
		std::vector<SDataKey> operands;
		OpParams params;
		while (std::getline(issArgs, curArg, ','))
		{
			applyWordTrim(curArg);
			if (!curArg.empty())
			{
				if (curArg[0] == ':')
				{ // variable
					std::map<std::string, SDataKey>::const_iterator curVar = vars.find(curArg);
					if (curVar != vars.end())
					{ // existing varible
						operands.push_back(curVar->second);
					}
					else
					{ // new variable
						SDataKey key = CTasksCreator::getUniqueDatafile();
						vars[curArg] = key;
						operands.push_back(key);
					}
				}
				else // number
					params.push_back(atof(curArg.c_str()));
			}
			if (issArgs.eof())
				break;
		}
		UniqueRequest request = RequestsFactory::getFromString(cmd, operands, params);
		if (request->getType() == IRequest::Type::UNSUPPORTED)
		{
			Log("Error while parsing script.");
			return 3;
		}
		manager.sendDependentRequest(std::move(request));
		if (issCmd.eof())
			break;
	}
	
	infile.close();

	// special case for example 1 to test IF
	CorrelationID ifID = "test-if";
	manager.sendDependentRequest(RequestsFactory::If(vars[":c"], 1, IRequest::COND_POS), ifID)
		   .saveContext()
		   .sendDependentRequest(RequestsFactory::Inc(vars[":a"]), manager.getUniqueCorrelationID(),
				   CCallbackRequest::formCallbackName(ifID, c_sTrue))
		   .restoreContext();

	manager.run();
	return 0;
}
