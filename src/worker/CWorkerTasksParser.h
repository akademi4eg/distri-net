#pragma once
#include "../SimplePocoHandler.h"
#include "../Protocol.h"
#include <memory>

class CWorkerTasksParser {
	SimplePocoHandler *pConnectionHandler;
	AMQP::Connection *pConnection;
	AMQP::Channel *pChannel;
public:
	CWorkerTasksParser(const std::string& host, uint16_t port,
			const AMQP::Login& login, const std::string& vhost);
	virtual ~CWorkerTasksParser();

	void run();
	std::unique_ptr<IRequest> parseRequest(const std::string& msg) const;
	std::unique_ptr<IResponse> processRequest(std::unique_ptr<IRequest> const & request);
};
