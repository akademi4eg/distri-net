#include <iostream>

#include "CTasksCreator.h"
#include "../Operations.h"
#define Log(x) (std::cout << x << std::endl)

int main(int argc, const char* argv[])
{
	CTasksCreator manager("localhost", 5672, AMQP::Login("dnet", "111111"), "/dnet");
	SDataKey key;
	key.sSource = "datafile";
	key.iIndex = 0;
	key.iEntrySize = 10;
	Operations::Type op = Operations::Type::DECREMENT;
	manager.sendRequest(std::unique_ptr<IRequest>(new CVersionRequest()));
	manager.sendRequest(std::unique_ptr<IRequest>(new CUnaryOpRequest(key, op)));
	manager.sendRequest(std::unique_ptr<IRequest>(new CExitRequest()));
	manager.run();
	return 0;
}
