#pragma once

namespace Operations
{
enum UnaryType
{
	ZEROS, INCREMENT, DECREMENT, FLIP_SIGN, UNARY_UNSUPPORTED
};
enum BinaryType
{
	ADD, BINARY_UNSUPPORTED
};

void increment(double& val);
void decrement(double& val);
void flipSign(double& val);

void add(double& base, double& other);
}
