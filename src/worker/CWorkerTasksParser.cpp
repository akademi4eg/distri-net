#include "../Operations.h"
#include "CWorkerTasksParser.h"
#include <string>
#include <sstream>

#define Log(x) (std::cout << x << std::endl)

CWorkerTasksParser::CWorkerTasksParser(const std::string& host, uint16_t port,
		const AMQP::Login& login, const std::string& vhost)
{
	Log("Setting up connection to Rabbit.");
	pConnectionHandler = new SimplePocoHandler(host, port);
	pConnection = new AMQP::Connection(pConnectionHandler, login, vhost);
	pChannel = new AMQP::Channel(pConnection);
	pChannel->setQos(1);

	pChannel->consume("batches_tasks").onReceived([&](const AMQP::Message &message,
            uint64_t deliveryTag,
            bool redelivered)
    {
        const auto body = message.message();
        Log("Got message! Parsing...");
        std::unique_ptr<IRequest> request = parseRequest(body);
        Log("Parsed message: " + request->toPrettyString());
        std::unique_ptr<IResponse> response = processRequest(request);
        if (request->isResponseRequired())
		{
            AMQP::Envelope env(response->toString());
            env.setCorrelationID(message.correlationID());
            pChannel->publish("", message.replyTo(), env);
		}
        pChannel->ack(deliveryTag);
    });

    Log("Worker is ready.");
}

CWorkerTasksParser::~CWorkerTasksParser()
{
	delete pChannel;
	delete pConnection;
	delete pConnectionHandler;
}

void CWorkerTasksParser::run()
{
	Log("Worker started.");
	pConnectionHandler->loop();
	Log("Worker terminated.");
}

std::unique_ptr<IRequest> CWorkerTasksParser::parseRequest(const std::string& msg) const
{
	std::istringstream msgStream(msg);
	std::string line;
	msgStream >> line;
	if (line == c_sUnaryOp)
	{
		SDataKey key;
		int op;
		msgStream >> key.sSource >> key.iIndex >> key.iEntrySize >> op;
		return std::unique_ptr<IRequest>(new CUnaryOpRequest(key, (Operations::Type)op));
	}
	else if (line == c_sVersion)
	{
		return std::unique_ptr<IRequest>(new CVersionRequest());
	}
	else if (line == c_sExit)
	{
		return std::unique_ptr<IRequest>(new CExitRequest());
	}

	return std::unique_ptr<IRequest>(new CUnsupportedRequest());
}

std::unique_ptr<IResponse> CWorkerTasksParser::processRequest(std::unique_ptr<IRequest>  const & request)
{
	switch (request->getType())
	{
	case IRequest::Type::UNARY_OP:
	{
		CUnaryOpRequest *unaryOp = reinterpret_cast<CUnaryOpRequest*>(request.get());
		if (applyUnaryOp(unaryOp->getKey(), unaryOp->getOp()))
			return std::unique_ptr<IResponse>(new CSuccessResponse());
		else
			return std::unique_ptr<IResponse>(new CErrorResponse("Failed to apply unary operation."));
	}
	case IRequest::Type::EXIT:
		pConnectionHandler->quit();
		return std::unique_ptr<IResponse>(new CSuccessResponse());
	case IRequest::Type::VERSION:
		return std::unique_ptr<IResponse>(new CVersionResponse());
	default:
		return std::unique_ptr<IResponse>(new CErrorResponse("Unsupported request."));
	}
}

bool CWorkerTasksParser::applyUnaryOp(const SDataKey& key, Operations::Type op)
{
	std::unique_ptr<DataEntry> data = fileReader.loadData(key);
	if (!data)
		return false;
	switch (op)
	{
	case Operations::Type::INCREMENT:
		std::for_each(data->begin(), data->end(), Operations::increment);
		break;
	case Operations::Type::DECREMENT:
		std::for_each(data->begin(), data->end(), Operations::decrement);
		break;
	case Operations::Type::FLIP_SIGN:
		std::for_each(data->begin(), data->end(), Operations::flipSign);
		break;
	default:
		return false;
	}
	return fileReader.saveData(key, *data);
}
