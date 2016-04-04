#include <cstdlib>

#include "CTasksCreator.h"
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Exception.h>
#include <Poco/Net/HTTPBasicCredentials.h>

#define Log(x) (std::cout << x << std::endl)

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

std::string CTasksCreator::getUniqueCorrelationID()
{
	static uint64_t uid = 1;
	return std::to_string(uid++);
}

void CTasksCreator::sendDependentRequest(
		std::unique_ptr<IRequest> request, const CorrelationID& corrID)
{
	sendRequest(applyDependencies(std::move(request)), corrID);
}

// TODO add a way to publish to callback queues
void CTasksCreator::sendRequest(
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
			dependencies[key.toString()] = idForDependencies;
		}
	}
	pChannel->publish(sBatchExc, sTaskRoutKey, env);
	Log(
			"Sent message [" + env.correlationID() + "]: "
					+ request->toPrettyString());
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

void CTasksCreator::clearRequest(const CorrelationID& corrID)
{
	std::string srcQueue(CCallbackRequest::formCallbackName(corrID));
	std::string apiCall("/api/parameters/shovel/%2f"+AMQPVHost.substr(1)+"/"+srcQueue);
	try
	{
		Poco::Net::HTTPBasicCredentials creds(AMQPCreds.user(), AMQPCreds.password());
		Poco::Net::HTTPClientSession session(AMQPHost, c_iHTTPRabbitPort);
		Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_DELETE, apiCall);
		creds.authenticate(request);
		session.sendRequest(request);
		Poco::Net::HTTPResponse response;
		std::istream& is = session.receiveResponse(response);
		std::string line;
		while (!is.eof())
			is >> line;
	}
    catch (Poco::Exception &ex)
    {
        Log("Failed to remove callback for " + corrID + ": " + ex.displayText());
    }
	pChannel->removeQueue(srcQueue).onSuccess([&](int){
		// if there are no request left, quit
		if (requestsSent.empty())
			pConnection->close();
	});
}
