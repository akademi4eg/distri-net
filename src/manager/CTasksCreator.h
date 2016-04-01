#pragma once

#include "../SimplePocoHandler.h"
#include "../Protocol.h"
#include <map>

class CTasksCreator {
	SimplePocoHandler *pConnectionHandler;
	AMQP::Connection *pConnection;
	AMQP::Channel *pChannel;
	std::string sResponsesQueue;
	std::string sBatchExc;
	std::string sTaskRoutKey;

	std::map<std::string, CorrelationID> dependencies;
public:
	CTasksCreator(const std::string& host, uint16_t port,
			const AMQP::Login& login, const std::string& vhost);
	virtual ~CTasksCreator();

	void run();
	std::string sendRequest(std::unique_ptr<IRequest> const & request,
			const std::string& corrID = getUniqueCorrelationID());
	std::unique_ptr<IRequest> applyDependencies(std::unique_ptr<IRequest> request);

	static std::string getUniqueCorrelationID();
};
