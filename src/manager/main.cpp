#include <iostream>

#include "CTasksCreator.h"
#include "../RequestsFactory.h"
#include "../Version.h"
#define Log(x) (std::cout << x << std::endl)

int main(int argc, const char* argv[])
{
	Log("Manager version "+c_sProductVersion);
	CTasksCreator manager("localhost", 5672, AMQP::Login("dnet", "111111"),
			"/dnet");
	/**
	 * Execute simple program:
	 * a = [1, 2, 3]
	 * b = [0, 0, 0]
	 * a = a-1
	 * b = b+1
	 * a = a+b
	 * c = b
	 * b = b+1
	 * a = a/b
	 * Result:
	 * a = [0.5 1 1.5]
	 * b = [2 2 2]
	 * c = [1 1 1]
	 */
	SDataKey key = CTasksCreator::getUniqueDatafile();
	SDataKey key2 = CTasksCreator::getUniqueDatafile();
	SDataKey key3 = CTasksCreator::getUniqueDatafile();
	size_t arSize = 3;
	manager.sendRequest(
		RequestsFactory::Set(key, OpParams{ 1, 2, 3 })).sendRequest(
		RequestsFactory::Zeros(key2, arSize)).sendDependentRequest(
		RequestsFactory::Dec(key)).sendDependentRequest(
		RequestsFactory::Inc(key2)).sendDependentRequest(
		RequestsFactory::Add(key, key2)).sendDependentRequest(
		RequestsFactory::Copy(key3, key2)).sendDependentRequest(
		RequestsFactory::Inc(key2)).sendDependentRequest(
		RequestsFactory::Div(key, key2));
	manager.run();
	return 0;
}
