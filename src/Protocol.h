#include "Version.h"

const std::string c_sWorker = "worker:";
const std::string c_sVersion = "VERSION";
const std::string c_sExit = "EXIT";
const std::string c_sUnsup = "UNSUPPORTED";
const std::string c_sSuccess = "OK";

class IRequest
{
public:
	enum class Type {VERSION, EXIT, UNSUPPORTED};
	virtual Type getType() const = 0;
	virtual std::string toString() const = 0;

	virtual bool isResponseRequired() const {return false;};
	virtual ~IRequest(){};
};

class CVersionRequest : public IRequest
{
public:
	Type getType() const {return Type::VERSION;};
	std::string toString() const {return c_sVersion;};
	bool isResponseRequired() const {return true;};
};

class CExitRequest : public IRequest
{
public:
	Type getType() const {return Type::EXIT;};
	std::string toString() const {return c_sExit;};
};

class CUnsupportedRequest : public IRequest
{
public:
	Type getType() const {return Type::UNSUPPORTED;};
	std::string toString() const {return c_sUnsup;};
	bool isResponseRequired() const {return true;};
};

class IResponse
{
public:
	virtual std::string toString() const = 0;

	virtual ~IResponse(){};
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
