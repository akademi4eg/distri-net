#pragma once

#include "../SimplePocoHandler.h"
#include "../Protocol.h"

class CTasksCreator {
	SimplePocoHandler *pConnectionHandler;
	AMQP::Connection *pConnection;
	AMQP::Channel *pChannel;
	std::string sResponsesQueue;
	std::string sBatchExc;
	std::string sTaskRoutKey;
public:
	CTasksCreator(const std::string& host, uint16_t port,
			const AMQP::Login& login, const std::string& vhost);
	virtual ~CTasksCreator();

	void run();
	std::string sendRequest(std::unique_ptr<IRequest> const & request,
			const std::string& corrID = getUniqueCorrelationID());

	static std::string getUniqueCorrelationID();
};
