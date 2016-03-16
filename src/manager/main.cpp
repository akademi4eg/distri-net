#include <iostream>

#include "CTasksCreator.h"
#define Log(x) (std::cout << x << std::endl)

int main(int argc, const char* argv[])
{
	CTasksCreator manager("localhost", 5672, AMQP::Login("dnet", "111111"), "/dnet");
	manager.sendRequest(std::unique_ptr<IRequest>(new CVersionRequest()));
	manager.sendRequest(std::unique_ptr<IRequest>(new CExitRequest()));
	manager.run();
	return 0;
}
