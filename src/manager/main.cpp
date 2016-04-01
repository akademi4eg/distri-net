#include <iostream>

#include "CTasksCreator.h"
#include "../Operations.h"
#define Log(x) (std::cout << x << std::endl)

int main(int argc, const char* argv[])
{
	CTasksCreator manager("localhost", 5672, AMQP::Login("dnet", "111111"),
			"/dnet");
	/**
	 * Execute simple program:
	 * a = 0
	 * b = 0
	 * b = b+1
	 * a = a+b
	 * b = b+1
	 */
	SDataKey key;
	key.sSource = "datafile";
	key.iIndex = 0;
	SDataKey key2;
	key2.sSource = "datafile2";
	key2.iIndex = 1;
	int arSize = 5;
	manager.sendRequest(
			std::unique_ptr<IRequest>(
					new CUnaryOpRequest(key, Operations::UnaryType::ZEROS,
							OpParams(1, arSize))));
	manager.sendRequest(
			std::unique_ptr<IRequest>(
					new CUnaryOpRequest(key2, Operations::UnaryType::ZEROS,
							OpParams(1, arSize))));
	manager.sendRequest(
			manager.applyDependencies(
					std::unique_ptr<IRequest>(
							new CUnaryOpRequest(key2,
									Operations::UnaryType::INCREMENT))));
	manager.sendRequest(
			manager.applyDependencies(
					std::unique_ptr<IRequest>(
							new CBinaryOpRequest(key, key2,
									Operations::BinaryType::ADD))));
	manager.sendRequest(
			manager.applyDependencies(
					std::unique_ptr<IRequest>(
							new CUnaryOpRequest(key2,
									Operations::UnaryType::INCREMENT))));
	manager.run();
	return 0;
}
