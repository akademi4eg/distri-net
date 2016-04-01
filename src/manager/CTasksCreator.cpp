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

	auto receiveCallback =
			[&](const AMQP::Message &message,
					uint64_t deliveryTag,
					bool redelivered)
			{
				Log("Got response [" + message.correlationID() + "]: " + message.message());
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

std::string CTasksCreator::getUniqueCorrelationID()
{
	return std::to_string(std::rand());
}

// TODO add a way to publish to callback queues
std::string CTasksCreator::sendRequest(
		std::unique_ptr<IRequest> const & request, const std::string& corrID)
{
	AMQP::Envelope env(request->toString());
	env.setCorrelationID(corrID);
	env.setReplyTo(sResponsesQueue);
	CorrelationID idForDependencies;
	if (request->getType() == IRequest::Type::CALLBACK)
		idForDependencies =
				reinterpret_cast<CCallbackRequest*>(request.get())->getBaseCorrelationID();
	else
		idForDependencies = env.correlationID();
	pChannel->declareQueue(
			CCallbackRequest::formCallbackName(idForDependencies),
			AMQP::durable);
	for (const SDataKey& key : request->getAffectedData())
	{
		dependencies[key.toString()] = idForDependencies;
	}
	pChannel->publish(sBatchExc, sTaskRoutKey, env);
	Log(
			"Sent message [" + env.correlationID() + "]: "
					+ request->toPrettyString());

	return corrID;
}

std::unique_ptr<IRequest> CTasksCreator::applyDependencies(
		std::unique_ptr<IRequest> request)
{
	switch (request->getType())
	{
	case IRequest::Type::UNARY_OP:
	{
		CUnaryOpRequest *curReq =
				reinterpret_cast<CUnaryOpRequest*>(request.get());
		std::map<std::string, CorrelationID>::const_iterator dependsOn =
				dependencies.find(curReq->getKey().toString());
		if (dependsOn != dependencies.end())
		{
			return std::unique_ptr<IRequest>(
					new CCallbackRequest(getUniqueCorrelationID(),
							dependsOn->second, std::move(request)));
		}
		break;
	}
	case IRequest::Type::BINARY_OP:
	{
		std::unique_ptr<IRequest> result;
		CBinaryOpRequest *curReq =
				reinterpret_cast<CBinaryOpRequest*>(request.get());
		std::map<std::string, CorrelationID>::const_iterator dependsOn =
				dependencies.find(curReq->getBaseKey().toString());
		if (dependsOn != dependencies.end())
		{
			result = std::unique_ptr<IRequest>(
					new CCallbackRequest(getUniqueCorrelationID(),
							dependsOn->second, std::move(request)));
		}
		else
			result = std::move(request);

		dependsOn = dependencies.find(curReq->getOtherKey().toString());
		if (dependsOn != dependencies.end())
		{
			result = std::unique_ptr<IRequest>(
					new CCallbackRequest(getUniqueCorrelationID(),
							dependsOn->second, std::move(result)));
		}
		return result;
	}
	case IRequest::Type::CALLBACK:
	{
		// preserve already existing callbacks
		CCallbackRequest *curReq =
				reinterpret_cast<CCallbackRequest*>(request.get());
		return std::unique_ptr<IRequest>(
				new CCallbackRequest(curReq->getCorrelationID(),
						curReq->getDependency(),
						applyDependencies(curReq->getCall())));
	}
	default:
		break;
	}
	return request;
}
