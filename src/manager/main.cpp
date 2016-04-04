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
	SDataKey key, key2, key3;
	key.sSource = "datafile";
	key.iIndex = 0;
	key2.sSource = "datafile2";
	key2.iIndex = 0;
	key3.sSource = "datafile3";
	key3.iIndex = 0;
	int arSize = 3;
	manager.sendRequest(
			std::unique_ptr<IRequest>(
					new CUnaryOpRequest(key, Operations::UnaryType::SET,
							OpParams{1, 2, 3})));
	manager.sendRequest(
			std::unique_ptr<IRequest>(
					new CUnaryOpRequest(key2, Operations::UnaryType::ZEROS,
							OpParams(1, arSize))));
	manager.sendDependentRequest(
					std::unique_ptr<IRequest>(
							new CUnaryOpRequest(key,
									Operations::UnaryType::DECREMENT)));
	manager.sendDependentRequest(
					std::unique_ptr<IRequest>(
							new CUnaryOpRequest(key2,
									Operations::UnaryType::INCREMENT)));
	manager.sendDependentRequest(
					std::unique_ptr<IRequest>(
							new CBinaryOpRequest(key, key2,
									Operations::BinaryType::ADD)));
	manager.sendDependentRequest(
					std::unique_ptr<IRequest>(
							new CBinaryOpRequest(key3, key2,
									Operations::BinaryType::COPY)));
	manager.sendDependentRequest(
					std::unique_ptr<IRequest>(
							new CUnaryOpRequest(key2,
									Operations::UnaryType::INCREMENT)));
	manager.sendDependentRequest(
					std::unique_ptr<IRequest>(
							new CBinaryOpRequest(key, key2,
									Operations::BinaryType::DIV)));
	manager.run();
	return 0;
}
