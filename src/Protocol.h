#include "Version.h"
#include "IDataReader.h"
#include "Operations.h"

static const char *c_UnaryOps[] = {"ZEROS", "INCREMENT", "DECREMENT", "FLIP_SIGN"};
static const char *c_BinaryOps[] = {"ADD"};
const std::string c_sWorker = "worker:";
const std::string c_sCallback = "CALLBACK";
const std::string c_sUnaryOp = "UNARY_OP";
const std::string c_sBinaryOp = "BINARY_OP";
const std::string c_sVersion = "VERSION";
const std::string c_sExit = "EXIT";
const std::string c_sUnsup = "UNSUPPORTED";
const std::string c_sSuccess = "OK";

class IRequest
{
public:
	enum class Type {CALLBACK, UNARY_OP, BINARY_OP, VERSION, EXIT, UNSUPPORTED};
	virtual Type getType() const = 0;
	virtual std::string toString() const = 0;
	virtual std::string toPrettyString() const {return toString();};
	virtual bool isReadOnly() const {return false;};

	virtual bool isResponseRequired() const {return false;};
	virtual ~IRequest(){};
};

class CCallbackRequest : public IRequest
{
	std::string callCorrelationID;
	std::string waitFor;
	std::unique_ptr<IRequest> call;

public:
	CCallbackRequest(const std::string& correlationID, const std::string& dependency,
			std::unique_ptr<IRequest> req)
		: callCorrelationID(correlationID), waitFor(dependency), call(std::move(req)) {}
	Type getType() const {return Type::CALLBACK;}
	std::string toString() const
	{
		return c_sCallback + "\n" + waitFor + "\n" + callCorrelationID + "\n" + call->toString();
	}
	std::string toPrettyString()
	{
		return "[call " + callCorrelationID + " after " + waitFor + "]: " + call->toPrettyString();
	}
	std::string getDependency() const {return waitFor;}
	std::string getCorrelationID() const {return callCorrelationID;}
	std::string getBaseCorrelationID() const
	{
		if (call->getType() == Type::CALLBACK)
			return reinterpret_cast<CCallbackRequest*>(call.get())->getBaseCorrelationID();
		else
			return getCorrelationID();
	}
	std::unique_ptr<IRequest> getCall() {return std::move(call);}
	bool isReadOnly() const {return true;};
};

class CUnaryOpRequest : public IRequest
{
	SDataKey key;
	Operations::UnaryType op;
public:
	CUnaryOpRequest(const SDataKey& akey, Operations::UnaryType aop)
		: key(akey), op(aop) {};
	Type getType() const {return Type::UNARY_OP;};
	std::string toString() const
	{
		return c_sUnaryOp + "\n" + key.toString() + "\n" + std::to_string(op);
	};
	std::string toPrettyString() const
	{
		std::string opStr((op<Operations::UnaryType::UNARY_UNSUPPORTED)?c_UnaryOps[op]:"UNSUPPORTED");
		return opStr + " for key " + key.toPrettyString();
	};
	bool isResponseRequired() const {return true;};
	SDataKey getKey() {return key;};
	Operations::UnaryType getOp() const {return op;};
};

class CBinaryOpRequest : public IRequest
{
	SDataKey keyBase;
	SDataKey keyOther;
	Operations::BinaryType op;
public:
	CBinaryOpRequest(const SDataKey& base, const SDataKey& other, Operations::BinaryType aop)
		: keyBase(base), keyOther(other), op(aop) {};
	Type getType() const {return Type::BINARY_OP;};
	std::string toString() const
	{
		return c_sBinaryOp + "\n" + keyBase.toString() + "\n" + keyOther.toString() + "\n" + std::to_string(op);
	};
	std::string toPrettyString() const
	{
		std::string opStr((op<Operations::BinaryType::BINARY_UNSUPPORTED)?c_BinaryOps[op]:"UNSUPPORTED");
		return opStr + " for base key " + keyBase.toPrettyString() + " and other key " + keyOther.toPrettyString();
	};
	bool isResponseRequired() const {return true;};
	SDataKey getBaseKey() {return keyBase;};
	SDataKey getOtherKey() {return keyOther;};
	Operations::BinaryType getOp() const {return op;};
};

class CVersionRequest : public IRequest
{
public:
	Type getType() const {return Type::VERSION;};
	std::string toString() const {return c_sVersion;};
	bool isResponseRequired() const {return true;};
	bool isReadOnly() const {return true;};
};

class CExitRequest : public IRequest
{
public:
	Type getType() const {return Type::EXIT;};
	std::string toString() const {return c_sExit;};
	bool isReadOnly() const {return true;};
};

class CUnsupportedRequest : public IRequest
{
public:
	Type getType() const {return Type::UNSUPPORTED;};
	std::string toString() const {return c_sUnsup;};
	bool isResponseRequired() const {return true;};
	bool isReadOnly() const {return true;};
};

class IResponse
{
public:
	virtual std::string toString() const = 0;

	virtual ~IResponse(){};
};

class CCallbackResponse : public IResponse
{
	std::string waitFor;
	std::string correlationID;
	std::unique_ptr<IRequest> call;
public:
	CCallbackResponse(const std::string& dependency, const std::string& corrID,
			std::unique_ptr<IRequest> request)
		: waitFor(dependency), correlationID(corrID), call(std::move(request)) {}
	std::string getDependency() const {return waitFor;}
	std::string getCorrelationID() const {return correlationID;}
	std::string toString() const {return call->toString();}
};

class CVersionResponse : public IResponse
{
public:
	std::string toString() const {return c_sWorker+c_sProduceVersion;};
};

class CErrorResponse : public IResponse
{
	std::string sErrMsg;
public:
	CErrorResponse(const std::string& msg) : sErrMsg(msg) {};
	std::string toString() const {return sErrMsg;};
};

class CSuccessResponse : public IResponse
{
public:
	std::string toString() const {return c_sSuccess;};
};
