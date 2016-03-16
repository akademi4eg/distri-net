#include "CWorkerTasksParser.h"

#define Log(x) (std::cout << x << std::endl)

CWorkerTasksParser::CWorkerTasksParser(const std::string& host, uint16_t port,
		const AMQP::Login& login, const std::string& vhost) {
	Log("Setting up connection to Rabbit.");
	pConnectionHandler = new SimplePocoHandler(host, port);
	pConnection = new AMQP::Connection(pConnectionHandler, login, vhost);
	pInputChannel = new AMQP::Channel(pConnection);
	pInputChannel->setQos(1);

	pInputChannel->consume("batches_tasks").onReceived([&](const AMQP::Message &message,
            uint64_t deliveryTag,
            bool redelivered)
    {
        const auto body = message.message();
        Log("Get message: " + body);
        std::unique_ptr<IRequest> request = parseRequest(body);
        std::unique_ptr<IResponse> response = processRequest(request);
        if (request->isResponseRequired())
		{
            AMQP::Envelope env(response->toString());
            env.setCorrelationID(message.correlationID());
            pInputChannel->publish("", message.replyTo(), env);
		}
        pInputChannel->ack(deliveryTag);
    });

    Log("Worker is ready.");
}

CWorkerTasksParser::~CWorkerTasksParser() {
	delete pInputChannel;
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
	if (msg == c_sVersion)
	{
		return std::unique_ptr<IRequest>(new CVersionRequest());
	}
	else if (msg == c_sExit)
	{
		return std::unique_ptr<IRequest>(new CExitRequest());
	}

	return std::unique_ptr<IRequest>(new CUnsupportedRequest());
}

std::unique_ptr<IResponse> CWorkerTasksParser::processRequest(std::unique_ptr<IRequest>  const & request)
{
	switch (request->getType())
	{
	case IRequest::Type::EXIT:
		pConnectionHandler->quit();
		return std::unique_ptr<IResponse>(new CSuccessResponse());
		break;
	case IRequest::Type::VERSION:
		return std::unique_ptr<IResponse>(new CVersionResponse());
		break;
	default:
		return std::unique_ptr<IResponse>(new CErrorResponse("Unsupported request."));
	}
}
