#include "../Operations.h"
#include "CWorkerTasksParser.h"
#include <string>
#include <sstream>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Exception.h>
#include <Poco/Net/HTTPBasicCredentials.h>

#define Log(x) (std::cout << x << std::endl)

CWorkerTasksParser::CWorkerTasksParser(const std::string& host, uint16_t port,
		const AMQP::Login& login, const std::string& vhost)
: AMQPCreds(login), AMQPHost(host), AMQPPort(port), AMQPVHost(vhost)
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
        Log("Got message [" + message.correlationID() + "]! Parsing...");
        std::unique_ptr<IRequest> request = parseRequest(body);
        Log("Parsed message: " + request->toPrettyString());
        std::unique_ptr<IResponse> response = processRequest(request);
        if (request->isResponseRequired())
		{
            AMQP::Envelope env(response->toString());
            env.setCorrelationID(message.correlationID());
            pChannel->publish("", message.replyTo(), env);
            Log("Sent response [" + env.correlationID() + "]: " + response->toString());
		}

        if (request->getType() == IRequest::Type::CALLBACK)
        {
        	CCallbackResponse *resp = reinterpret_cast<CCallbackResponse*>(response.get());
            AMQP::Envelope env(resp->toString());
            env.setCorrelationID(resp->getCorrelationID());
            env.setReplyTo(message.replyTo());
            std::string callQueue = c_sCallback + "-" + resp->getDependency();
            pChannel->publish("", callQueue, env);
            Log("Sent callback to " + callQueue + ": " + resp->toString());
        }

        if (!request->isReadOnly())
        {
        	Log("Allowing execution of calls waiting for " + message.correlationID());
        	allowCallbacksProcessing(message.correlationID());
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

void CWorkerTasksParser::allowCallbacksProcessing(const std::string& corrID)
{
	std::string uri("amqp://"+AMQPCreds.user()+"@/%2f"+AMQPVHost.substr(1));
	std::string srcQueue(c_sCallback + "-" + corrID);
	std::string apiCall("/api/parameters/shovel/%2f"+AMQPVHost.substr(1)+"/"+srcQueue);
	std::string cmd("{\"value\": {\"src-uri\":  \""+uri+"\", \"src-queue\": \""+srcQueue+"\",\"dest-uri\": \""+uri+"\", \"dest-queue\": \"batches_tasks\"}}");
	try
	{
		Poco::Net::HTTPBasicCredentials creds(AMQPCreds.user(), AMQPCreds.password());
		Poco::Net::HTTPClientSession session(AMQPHost, 15672);
		Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_PUT, apiCall);
		request.setContentType("application/json");
		request.setContentLength(cmd.size());
		creds.authenticate(request);
		std::ostream& os = session.sendRequest(request);
		std::istringstream ss_in(cmd);
		Poco::StreamCopier::copyStream(ss_in, os);
	}
    catch (Poco::Exception &ex)
    {
        Log("Failed to update callbacks: " + ex.displayText());
    }
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
		return std::unique_ptr<IRequest>(new CUnaryOpRequest(key, (Operations::UnaryType)op));
	}
	else if (line == c_sBinaryOp)
	{
		SDataKey keyBase;
		SDataKey keyOther;
		int op;
		msgStream >> keyBase.sSource >> keyBase.iIndex >> keyBase.iEntrySize;
		msgStream >> keyOther.sSource >> keyOther.iIndex >> keyOther.iEntrySize >> op;
		return std::unique_ptr<IRequest>(new CBinaryOpRequest(keyBase, keyOther, (Operations::BinaryType)op));
	}
	else if (line == c_sCallback)
	{
		std::string waitFor, correlationID;
		msgStream >> waitFor >> correlationID;
		std::string requestStr(msgStream.str().substr(msgStream.tellg()));
		return std::unique_ptr<IRequest>(new CCallbackRequest(correlationID, waitFor,
				parseRequest(requestStr)));
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
	case IRequest::Type::CALLBACK:
	{
		CCallbackRequest *callback = reinterpret_cast<CCallbackRequest*>(request.get());
		return std::unique_ptr<IResponse>(new CCallbackResponse(callback->getDependency(),
				callback->getCorrelationID(), std::move(callback->getCall())));
	}
	case IRequest::Type::UNARY_OP:
	{
		CUnaryOpRequest *unaryOp = reinterpret_cast<CUnaryOpRequest*>(request.get());
		if (applyUnaryOp(unaryOp->getKey(), unaryOp->getOp()))
			return std::unique_ptr<IResponse>(new CSuccessResponse());
		else
			return std::unique_ptr<IResponse>(new CErrorResponse("Failed to apply unary operation."));
	}
	case IRequest::Type::BINARY_OP:
	{
		CBinaryOpRequest *binaryOp = reinterpret_cast<CBinaryOpRequest*>(request.get());
		if (applyBinaryOp(binaryOp->getBaseKey(), binaryOp->getOtherKey(), binaryOp->getOp()))
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

bool CWorkerTasksParser::applyUnaryOp(const SDataKey& key, Operations::UnaryType op)
{
	std::unique_ptr<DataEntry> data;
	if (op != Operations::UnaryType::ZEROS)
	{
		data = fileReader.loadData(key);
		if (!data)
			return false;
	}
	switch (op)
	{
	case Operations::UnaryType::ZEROS:
		data = std::unique_ptr<DataEntry>(new DataEntry(key.iEntrySize, 0.0));
		break;
	case Operations::UnaryType::INCREMENT:
		std::for_each(data->begin(), data->end(), Operations::increment);
		break;
	case Operations::UnaryType::DECREMENT:
		std::for_each(data->begin(), data->end(), Operations::decrement);
		break;
	case Operations::UnaryType::FLIP_SIGN:
		std::for_each(data->begin(), data->end(), Operations::flipSign);
		break;
	default:
		return false;
	}
	return fileReader.saveData(key, *data);
}

bool CWorkerTasksParser::applyBinaryOp(const SDataKey& keyBase, const SDataKey& keyOther, Operations::BinaryType op)
{
	if (keyBase.iEntrySize != keyOther.iEntrySize)
		return false;
	std::unique_ptr<DataEntry> dataBase = fileReader.loadData(keyBase);
	if (!dataBase)
		return false;
	std::unique_ptr<DataEntry> dataOther = fileReader.loadData(keyOther);
	if (!dataOther)
		return false;
	switch (op)
	{
	case Operations::BinaryType::ADD:
	{
		DataEntry::iterator other = dataOther->begin();
		for (DataEntry::iterator base = dataBase->begin(); base != dataBase->end(); ++base, ++other)
		{
			Operations::add(*base, *other);
		}
		break;
	}
	default:
		return false;
	}
	return fileReader.saveData(keyBase, *dataBase);
}
