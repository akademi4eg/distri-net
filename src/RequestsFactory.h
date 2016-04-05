#pragma once

#include <memory>
#include "Protocol.h"

typedef std::unique_ptr<IRequest> UniqueRequest;

namespace RequestsFactory
{
UniqueRequest getFromString(const std::string& req,
				std::vector<SDataKey> keys, const OpParams& params)
{
	int i = 0;
	while (keys.size()==1 && i < Operations::UnaryType::UNARY_UNSUPPORTED)
	{
		if (req == c_UnaryOps[i])
		{
			// it is an unary operation
			return UniqueRequest(
				new CUnaryOpRequest(keys[0], (Operations::UnaryType)i, params));
		}
		i++;
	}
	i = 0;
	while (keys.size()==2 && i < Operations::BinaryType::BINARY_UNSUPPORTED)
	{
		if (req == c_BinaryOps[i])
		{
			// it is a binary operation
			return UniqueRequest(
				new CBinaryOpRequest(keys[0], keys[1], (Operations::BinaryType)i, params));
		}
		i++;
	}
	// some wrong request
	return UniqueRequest(new CUnsupportedRequest());
}
	
UniqueRequest Zeros(const SDataKey& key, size_t num)
{
	return UniqueRequest(new CUnaryOpRequest(key, Operations::UnaryType::ZEROS,
			OpParams(1, num)));
}

UniqueRequest Set(const SDataKey& key, const OpParams& vals)
{
	return UniqueRequest(new CUnaryOpRequest(key, Operations::UnaryType::SET,
			vals));
}

UniqueRequest Inc(const SDataKey& key)
{
	return UniqueRequest(new CUnaryOpRequest(key, Operations::UnaryType::INCREMENT));
}

UniqueRequest Dec(const SDataKey& key)
{
	return UniqueRequest(new CUnaryOpRequest(key, Operations::UnaryType::DECREMENT));
}

UniqueRequest FlipSign(const SDataKey& key)
{
	return UniqueRequest(new CUnaryOpRequest(key, Operations::UnaryType::FLIP_SIGN));
}

UniqueRequest Add(const SDataKey& keyBase, const SDataKey& keyOther)
{
	return UniqueRequest(new CBinaryOpRequest(keyBase, keyOther,
			Operations::BinaryType::ADD));
}

UniqueRequest Sub(const SDataKey& keyBase, const SDataKey& keyOther)
{
	return UniqueRequest(new CBinaryOpRequest(keyBase, keyOther,
			Operations::BinaryType::SUB));
}

UniqueRequest Mul(const SDataKey& keyBase, const SDataKey& keyOther)
{
	return UniqueRequest(new CBinaryOpRequest(keyBase, keyOther,
			Operations::BinaryType::MUL));
}

UniqueRequest Div(const SDataKey& keyBase, const SDataKey& keyOther)
{
	return UniqueRequest(new CBinaryOpRequest(keyBase, keyOther,
			Operations::BinaryType::DIV));
}

UniqueRequest Copy(const SDataKey& keyBase, const SDataKey& keyOther)
{
	return UniqueRequest(new CBinaryOpRequest(keyBase, keyOther,
			Operations::BinaryType::COPY));
}
}
