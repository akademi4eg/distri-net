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
	key.iEntrySize = 10;
	SDataKey key2;
	key2.sSource = "datafile2";
	key2.iIndex = 1;
	key2.iEntrySize = 10;
	std::string id1, id2, inc2id, preAddID, addID, finalID;
	inc2id = CTasksCreator::getUniqueCorrelationID();
	preAddID = CTasksCreator::getUniqueCorrelationID();
	addID = CTasksCreator::getUniqueCorrelationID();
	finalID = CTasksCreator::getUniqueCorrelationID();
	id1 = manager.sendRequest(
			std::unique_ptr<IRequest>(
					new CUnaryOpRequest(key, Operations::UnaryType::ZEROS)));
	id2 = manager.sendRequest(
			std::unique_ptr<IRequest>(
					new CUnaryOpRequest(key2, Operations::UnaryType::ZEROS)));
	manager.sendRequest(
			std::unique_ptr<IRequest>(
					new CCallbackRequest(inc2id, id2,
							std::unique_ptr<IRequest>(
									new CUnaryOpRequest(key2,
											Operations::UnaryType::INCREMENT)))));
	manager.sendRequest(
			std::unique_ptr<IRequest>(
					new CCallbackRequest(preAddID, id1,
							std::unique_ptr<IRequest>(
									new CCallbackRequest(addID, inc2id,
											std::unique_ptr<IRequest>(
													new CBinaryOpRequest(key,
															key2,
															Operations::BinaryType::ADD)))))));
	manager.sendRequest(
			std::unique_ptr<IRequest>(
					new CCallbackRequest(finalID, addID,
							std::unique_ptr<IRequest>(
									new CUnaryOpRequest(key2,
											Operations::UnaryType::INCREMENT)))));
	manager.run();
	return 0;
}
