#pragma once

#include "../SimplePocoHandler.h"
#include "../Protocol.h"
#include <map>
#include <list>
#include <stack>

typedef std::map<std::string, CorrelationID> DepsMap;

const std::string c_sBatchExc = "batches_ops";
const std::string c_sTaskRoutKey = "task";

class CTasksCreator {
	SimplePocoHandler *pConnectionHandler;
	AMQP::Connection *pConnection;
	AMQP::Channel *pChannel;
	AMQP::Login AMQPCreds;
	std::string AMQPHost;
	uint16_t AMQPPort;
	std::string AMQPVHost;

	std::string sResponsesQueue;

	DepsMap dependencies;
	std::list<CorrelationID> requestsSent;
	std::stack<DepsMap> depsStack;
public:
	CTasksCreator(const std::string& host, uint16_t port,
			const AMQP::Login& login, const std::string& vhost);
	virtual ~CTasksCreator();

	void run();
	CTasksCreator& sendDependentRequest(std::unique_ptr<IRequest> request,
			const CorrelationID& corrID = getUniqueCorrelationID(),
			const std::string& sendTo = c_sBatchExc);
	CTasksCreator& sendRequest(std::unique_ptr<IRequest> const & request,
			const CorrelationID& corrID = getUniqueCorrelationID(),
			const std::string& sendTo = c_sBatchExc);
	std::unique_ptr<IRequest> applyDependencies(std::unique_ptr<IRequest> request);
	CTasksCreator& saveContext() {depsStack.push(dependencies); return *this;};
	CTasksCreator& restoreContext() {dependencies = depsStack.top(); depsStack.pop(); return *this;};

	static std::string getUniqueCorrelationID();
	static SDataKey getUniqueDatafile();
private:
	void clearRequest(const CorrelationID& corrID);
};
