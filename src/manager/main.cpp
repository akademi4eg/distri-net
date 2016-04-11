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
	CorrelationID currParentCall;
	while (std::getline(infile, line))
	{
		if (line.substr(0, 2) == "//") // comment
			continue;
		std::istringstream issCmd(line);
		std::string cmd;
		issCmd >> cmd;
		applyWordTrim(cmd);
		if (cmd.empty()) // blank line
			continue;
		std::string curArg;
		std::vector<SDataKey> operands;
		OpParams params;
		while (std::getline(issCmd, curArg, ','))
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
			if (issCmd.eof())
				break;
		}
		UniqueRequest request = RequestsFactory::getFromString(cmd, operands, params);
		if (request->getType() == IRequest::Type::IF)
		{
			manager.sendDependentRequest(std::move(request));
			currParentCall = manager.getLastParentCall();
			manager.setCurrentQueue(CCallbackRequest::formCallbackName(currParentCall, c_sTrue))
				   .saveContext();
		}
		else if (cmd == c_sElse)
		{
			manager.sendRequest(RequestsFactory::EndIf(currParentCall))
			 	   .restoreContext(false)
				   .setCurrentQueue(CCallbackRequest::formCallbackName(currParentCall, c_sFalse))
				   .saveContext();
		}
		else if (cmd == c_sEndIf)
		{
			manager.sendRequest(RequestsFactory::EndIf(currParentCall))
				   .restoreContext();
		}
		else if (request->getType() == IRequest::Type::UNSUPPORTED)
		{
			Log("Error while parsing script.");
			return 3;
		}
		else
		{
			manager.sendDependentRequest(std::move(request));
		}
		if (infile.eof())
			break;
	}
	
	infile.close();

	manager.run();
	return 0;
}
