#pragma once

#include "../SimplePocoHandler.h"
#include "../Protocol.h"
#include <map>
#include <list>
#include <stack>

typedef std::map<std::string, CorrelationID> DepsMap;

const std::string c_sBatchExc = "batches_ops";
const std::string c_sTaskRoutKey = "task";

struct SContext
{
	DepsMap dependencies;
	std::string currQueue;
};

class CTasksCreator {
	SimplePocoHandler *pConnectionHandler;
	AMQP::Connection *pConnection;
	AMQP::Channel *pChannel;
	AMQP::Login AMQPCreds;
	std::string AMQPHost;
	uint16_t AMQPPort;
	std::string AMQPVHost;

	std::string sResponsesQueue;

	SContext context;
	std::list<CorrelationID> requestsSent;
	std::stack<SContext> appStack;
public:
	CTasksCreator(const std::string& host, uint16_t port,
			const AMQP::Login& login, const std::string& vhost);
	virtual ~CTasksCreator();

	void run();
	CTasksCreator& sendDependentRequest(std::unique_ptr<IRequest> request,
			const CorrelationID& corrID = getUniqueCorrelationID());
	CTasksCreator& sendRequest(std::unique_ptr<IRequest> const & request,
			const CorrelationID& corrID = getUniqueCorrelationID());
	std::unique_ptr<IRequest> applyDependencies(std::unique_ptr<IRequest> request);
	CTasksCreator& saveContext();
	CTasksCreator& restoreContext(bool doPop = true);
	CTasksCreator& setCurrentQueue(const std::string& queue = c_sBatchExc) {context.currQueue = queue; return *this;};
	const CorrelationID& getLastParentCall() const {return requestsSent.back();};

	static std::string getUniqueCorrelationID();
	static SDataKey getUniqueDatafile();
private:
	void clearRequest(const CorrelationID& corrID);
};
