#pragma once

#include <memory>
#include "Protocol.h"

typedef std::unique_ptr<IRequest> UniqueRequest;

namespace RequestsFactory
{
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
