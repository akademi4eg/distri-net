#pragma once

#include "../SimplePocoHandler.h"
#include "../Protocol.h"
#include <map>
#include <list>

class CTasksCreator {
	SimplePocoHandler *pConnectionHandler;
	AMQP::Connection *pConnection;
	AMQP::Channel *pChannel;
	AMQP::Login AMQPCreds;
	std::string AMQPHost;
	uint16_t AMQPPort;
	std::string AMQPVHost;

	std::string sResponsesQueue;
	std::string sBatchExc;
	std::string sTaskRoutKey;

	std::map<std::string, CorrelationID> dependencies;
	std::list<CorrelationID> requestsSent;
public:
	CTasksCreator(const std::string& host, uint16_t port,
			const AMQP::Login& login, const std::string& vhost);
	virtual ~CTasksCreator();

	void run();
	void sendDependentRequest(std::unique_ptr<IRequest> request,
			const CorrelationID& corrID = getUniqueCorrelationID());
	void sendRequest(std::unique_ptr<IRequest> const & request,
			const CorrelationID& corrID = getUniqueCorrelationID());
	std::unique_ptr<IRequest> applyDependencies(std::unique_ptr<IRequest> request);

	static std::string getUniqueCorrelationID();
private:
	void clearRequest(const CorrelationID& corrID);
};
