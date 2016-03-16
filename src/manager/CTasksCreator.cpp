#include <cstdlib>

#include "CTasksCreator.h"

#define Log(x) (std::cout << x << std::endl)

CTasksCreator::CTasksCreator(const std::string& host, uint16_t port,
		const AMQP::Login& login, const std::string& vhost)
{

	Log("Setting up connection to Rabbit.");
	pConnectionHandler = new SimplePocoHandler(host, port);
	pConnection = new AMQP::Connection(pConnectionHandler, login, vhost);
	pChannel = new AMQP::Channel(pConnection);
	pChannel->setQos(1);

	Log("Declaring responses queue.");
	sResponsesQueue = "resp";
	sBatchExc = "batches_ops";
	sTaskRoutKey = "task";
	pChannel->declareQueue(sResponsesQueue, AMQP::exclusive);

	auto receiveCallback = [&](const AMQP::Message &message,
			uint64_t deliveryTag,
			bool redelivered)
	{
		Log("Got response: " + message.message());
		pChannel->ack(deliveryTag);
	};

	pChannel->consume(sResponsesQueue).onReceived(receiveCallback);
	Log("Manager is ready.");
}

CTasksCreator::~CTasksCreator()
{
	delete pChannel;
	delete pConnection;
	delete pConnectionHandler;
}

void CTasksCreator::run()
{
	Log("Manager started.");
	pConnectionHandler->loop();
	Log("Manager terminated.");
}

void CTasksCreator::sendRequest(std::unique_ptr<IRequest> const & request)
{
	AMQP::Envelope env(request->toString());
	env.setCorrelationID(std::to_string(std::rand()));
	env.setReplyTo(sResponsesQueue);
	pChannel->publish(sBatchExc, sTaskRoutKey, env);
	Log("Sent " + request->toString() + " request.");
}
