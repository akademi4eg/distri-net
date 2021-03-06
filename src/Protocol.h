#pragma once

#include "Version.h"
#include "IDataReader.h"
#include "Operations.h"

typedef std::string CorrelationID;
typedef std::vector<double> OpParams;

static const char *c_Ifs[] = {"IFZ", "IFPOS", "IFNEG"};
static const char *c_UnaryOps[] = {"ZEROS", "SET", "INCREMENT", "DECREMENT", "FLIP_SIGN"};
static const char *c_BinaryOps[] = {"ADD", "SUB", "MUL", "DIV", "COPY"};
const std::string c_sWorker = "worker:";
const std::string c_sIf = "IF";
const std::string c_sElse = "ELSE";
const std::string c_sEndIf = "ENDIF";
const std::string c_sCallback = "CALLBACK";
const std::string c_sUnaryOp = "UNARY_OP";
const std::string c_sBinaryOp = "BINARY_OP";
const std::string c_sVersion = "VERSION";
const std::string c_sExit = "EXIT";
const std::string c_sUnsup = "UNSUPPORTED";
const std::string c_sSuccess = "OK";
const std::string c_sTrue = "TRUE";
const std::string c_sFalse = "FALSE";

class IRequest
{
public:
	enum class Type {IF, ENDIF, CALLBACK, UNARY_OP, BINARY_OP, VERSION, EXIT, UNSUPPORTED};
	virtual Type getType() const = 0;
	virtual std::string toString() const = 0;
	virtual std::string toPrettyString() const {return toString();};
	virtual bool isReadOnly() const {return false;};
	// TODO separate read and write dependencies
	virtual std::vector<SDataKey> getAffectedData() const {return std::vector<SDataKey>();};

	virtual bool isResponseRequired() const {return false;};
	virtual ~IRequest(){};
};

class CCallbackRequest : public IRequest
{
	CorrelationID callCorrelationID;
	CorrelationID waitFor;
	std::unique_ptr<IRequest> call;

public:
	static std::string formCallbackName(const CorrelationID& corrID, const std::string& prefix = c_sCallback) {return prefix + "-" + corrID;}

	CCallbackRequest(const CorrelationID& correlationID, const CorrelationID& dependency,
			std::unique_ptr<IRequest> req)
		: callCorrelationID(correlationID), waitFor(dependency), call(std::move(req)) {}
	Type getType() const {return Type::CALLBACK;}
	std::string toString() const
	{
		return c_sCallback + "\n" + waitFor + "\n" + callCorrelationID + "\n" + call->toString();
	}
	std::string toPrettyString() const
	{
		return "[call " + callCorrelationID + " after " + waitFor + "]: " + call->toPrettyString();
	}
	CorrelationID getDependency() const {return waitFor;}
	CorrelationID getCorrelationID() const {return callCorrelationID;}
	CorrelationID getBaseCorrelationID() const
	{
		if (call->getType() == Type::CALLBACK)
			return reinterpret_cast<CCallbackRequest*>(call.get())->getBaseCorrelationID();
		else
			return getCorrelationID();
	}
	std::unique_ptr<IRequest> getCall() {return std::move(call);}
	bool isReadOnly() const {return true;};
	std::vector<SDataKey> getAffectedData() const {return call->getAffectedData();};
};

class CEndIfRequest : public IRequest
{
	CorrelationID ifCorr;
public:
	CEndIfRequest(CorrelationID corrID) : ifCorr(corrID) {};
	CorrelationID getBlockCorrelationID() {return ifCorr;};
	Type getType() const {return Type::ENDIF;};
	std::string toString() const {return c_sEndIf + "\n" + ifCorr;};
	std::string toPrettyString() const {return "End IF for " + ifCorr;};
	bool isReadOnly() const {return true;};
};

class CIfRequest : public IRequest
{
	SDataKey key;
	size_t iIndex;
	Operations::Condition eCond;
public:
	CIfRequest(const SDataKey& akey, size_t idx,
			Operations::Condition cond)
		: key(akey), iIndex(idx), eCond(cond) {};
	Type getType() const {return Type::IF;};
	std::string toString() const
	{
		return c_sIf + "\n" + key.toString() + "\n" + std::to_string(iIndex) + "\n" + std::to_string(eCond);
	};
	std::string toPrettyString() const
	{
		return "Run "+ std::string(c_Ifs[eCond]) +" for key " + key.toPrettyString() + "@" + std::to_string(iIndex);
	};
	bool isReadOnly() const {return true;};
	SDataKey getKey() const {return key;};
	size_t getIndex() const {return iIndex;};
	Operations::Condition getCondition() const {return eCond;};
	std::vector<SDataKey> getAffectedData() const {return std::vector<SDataKey>(1, key);}
};

class CUnaryOpRequest : public IRequest
{
	SDataKey key;
	Operations::UnaryType op;
	OpParams params;
public:
	CUnaryOpRequest(const SDataKey& akey, Operations::UnaryType aop,
			const OpParams& opParams = OpParams())
		: key(akey), op(aop), params(opParams) {};
	Type getType() const {return Type::UNARY_OP;};
	std::string toString() const
	{
		std::string res(c_sUnaryOp + "\n" + key.toString() + "\n" + std::to_string(op));
		for (OpParams::value_type val : params)
			res += "\n" + std::to_string(val);
		return res;
	};
	std::string toPrettyString() const
	{
		std::string opStr((op<Operations::UnaryType::UNARY_UNSUPPORTED)?c_UnaryOps[op]:"UNSUPPORTED");
		return opStr + " for key " + key.toPrettyString();
	};
	bool isResponseRequired() const {return true;};
	SDataKey getKey() {return key;};
	Operations::UnaryType getOp() const {return op;};
	OpParams getParams() const {return params;}
	std::vector<SDataKey> getAffectedData() const {return std::vector<SDataKey>(1, key);}
};

class CBinaryOpRequest : public IRequest
{
	SDataKey keyBase;
	SDataKey keyOther;
	Operations::BinaryType op;
	OpParams params;
public:
	CBinaryOpRequest(const SDataKey& base, const SDataKey& other,
			Operations::BinaryType aop, const OpParams& opParams = OpParams())
		: keyBase(base), keyOther(other), op(aop), params(opParams) {};
	Type getType() const {return Type::BINARY_OP;};
	std::string toString() const
	{
		std::string res(c_sBinaryOp + "\n" + keyBase.toString() + "\n"
				+ keyOther.toString() + "\n" + std::to_string(op));
		for (OpParams::value_type val : params)
			res += "\n" + std::to_string(val);
		return res;
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
	OpParams getParams() const {return params;}
	std::vector<SDataKey> getAffectedData() const
	{
		std::vector<SDataKey> res(2);
		res.push_back(keyBase);
		res.push_back(keyOther);
		return res;
	}
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
	virtual std::string toPrettyString() const {return toString();};

	virtual ~IResponse(){};
};

class CIfResponse : public IResponse
{
	bool predicate;
public:
	CIfResponse(bool pred) : predicate(pred) {};
	virtual std::string toString() const {return predicate?c_sTrue:c_sFalse;};

	bool getResult() const {return predicate;};
};

class CEndIfResponse : public IResponse
{
	CorrelationID ifCorr;
public:
	CEndIfResponse(const CorrelationID& corrID) : ifCorr(corrID) {};
	virtual std::string toString() const {return ifCorr;};

	CorrelationID getBlockCorrelationID() const {return ifCorr;};
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
	std::string toPrettyString() const {return call->toPrettyString();}
};

class CVersionResponse : public IResponse
{
public:
	std::string toString() const {return c_sWorker+c_sProductVersion;};
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
