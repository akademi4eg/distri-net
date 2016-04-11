#include <cstdlib>

#include "CTasksCreator.h"

#define Log(x) (std::cout << x << std::endl)
const std::string c_sDatafilePrefix = "datafile-";

CTasksCreator::CTasksCreator(const std::string& host, uint16_t port,
		const AMQP::Login& login, const std::string& vhost)
	: AMQPCreds(login), AMQPHost(host), AMQPPort(port), AMQPVHost(vhost)
{

	Log("Setting up connection to Rabbit.");
	pConnectionHandler = new SimplePocoHandler(host, port);
	pConnection = new AMQP::Connection(pConnectionHandler, login, vhost);
	pChannel = new AMQP::Channel(pConnection);
	pChannel->setQos(1);

	Log("Declaring responses queue.");
	sResponsesQueue = "resp";
	pChannel->declareQueue(sResponsesQueue, AMQP::exclusive);
	context.currQueue = c_sBatchExc;

	auto receiveCallback =
			[&](const AMQP::Message &message,
					uint64_t deliveryTag,
					bool redelivered)
			{
				Log("Got response [" + message.correlationID() + "]: " + message.message());
				pChannel->ack(deliveryTag);

				if (message.correlationID() == requestsSent.back())
				{
					Log("Received last message response. Performing cleanup...");
					while (!requestsSent.empty())
					{
						CorrelationID corrID = requestsSent.front();
						requestsSent.pop_front();
						clearRequest(corrID);
					}
				}
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

SDataKey CTasksCreator::getUniqueDatafile()
{
	static uint64_t uid = 1;
	return SDataKey(c_sDatafilePrefix + std::to_string(uid++), 0);
}

std::string CTasksCreator::getUniqueCorrelationID()
{
	static uint64_t uid = 1;
	return std::to_string(uid++);
}

CTasksCreator& CTasksCreator::sendDependentRequest(
		std::unique_ptr<IRequest> request,
		const CorrelationID& corrID)
{
	return sendRequest(applyDependencies(std::move(request)), corrID);
}

CTasksCreator& CTasksCreator::sendRequest(
		std::unique_ptr<IRequest> const & request, const CorrelationID& corrID)
{
	AMQP::Envelope env(request->toString());
	env.setCorrelationID(corrID);
	env.setReplyTo(sResponsesQueue);
	std::vector<SDataKey> affectedData = request->getAffectedData();
	if (!affectedData.empty())
	{
		CorrelationID idForDependencies;
		if (request->getType() == IRequest::Type::CALLBACK)
			idForDependencies =
					reinterpret_cast<CCallbackRequest*>(request.get())->getBaseCorrelationID();
		else
			idForDependencies = env.correlationID();
		pChannel->declareQueue(
				CCallbackRequest::formCallbackName(idForDependencies),
				AMQP::durable);
		requestsSent.push_back(idForDependencies);
		for (const SDataKey& key : affectedData)
		{
			context.dependencies[key.toString()] = idForDependencies;
		}
	}
	// TODO update this to use routine keys and exchanges
	if (context.currQueue != c_sBatchExc)
	{
		pChannel->declareQueue(context.currQueue, AMQP::durable);
		pChannel->publish("", context.currQueue, env);
		Log(
			"Sent message [" + env.correlationID() + "] to " + context.currQueue + ": "
					+ request->toPrettyString());
	}
	else
	{
		pChannel->publish(context.currQueue, c_sTaskRoutKey, env);
		Log(
			"Sent message [" + env.correlationID() + "]: "
					+ request->toPrettyString());
	}
	return *this;
}

std::unique_ptr<IRequest> CTasksCreator::applyDependencies(
		std::unique_ptr<IRequest> request)
{
	switch (request->getType())
	{
	case IRequest::Type::IF:
	{
		CIfRequest *curReq =
				reinterpret_cast<CIfRequest*>(request.get());
		std::map<std::string, CorrelationID>::const_iterator dependsOn =
				context.dependencies.find(curReq->getKey().toString());
		if (dependsOn != context.dependencies.end())
		{
			return std::unique_ptr<IRequest>(
					new CCallbackRequest(getUniqueCorrelationID(),
							dependsOn->second, std::move(request)));
		}
		break;
	}
	case IRequest::Type::UNARY_OP:
	{
		CUnaryOpRequest *curReq =
				reinterpret_cast<CUnaryOpRequest*>(request.get());
		std::map<std::string, CorrelationID>::const_iterator dependsOn =
				context.dependencies.find(curReq->getKey().toString());
		if (dependsOn != context.dependencies.end())
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
				context.dependencies.find(curReq->getBaseKey().toString());
		if (dependsOn != context.dependencies.end())
		{
			result = std::unique_ptr<IRequest>(
					new CCallbackRequest(getUniqueCorrelationID(),
							dependsOn->second, std::move(request)));
		}
		else
			result = std::move(request);

		dependsOn = context.dependencies.find(curReq->getOtherKey().toString());
		if (dependsOn != context.dependencies.end())
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

void CTasksCreator::clearRequest(const CorrelationID& corrID)
{
	std::string srcQueue(CCallbackRequest::formCallbackName(corrID));
	SimplePocoHandler::removeShovel(srcQueue, AMQPHost, AMQPVHost, AMQPCreds.user(), AMQPCreds.password());
	pChannel->removeQueue(srcQueue).onSuccess([&](int){
		// if there are no request left, quit
		if (requestsSent.empty())
			pConnection->close();
	});
}

CTasksCreator& CTasksCreator::saveContext()
{
	appStack.push(context);
	return *this;
}


CTasksCreator& CTasksCreator::restoreContext(bool doPop)
{
	context = appStack.top();
	if (doPop)
		appStack.pop();
	return *this;
}
