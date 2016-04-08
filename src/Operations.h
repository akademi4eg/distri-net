#pragma once

namespace Operations
{
enum UnaryType
{
	ZEROS, SET, INCREMENT, DECREMENT, FLIP_SIGN, UNARY_UNSUPPORTED
};
enum BinaryType
{
	ADD, SUB, MUL, DIV, COPY, BINARY_UNSUPPORTED
};
enum Condition
{
	COND_ZERO, COND_POS, COND_NEG, COND_UNSUPPORTED
};

void increment(double& val);
void decrement(double& val);
void flipSign(double& val);

void add(double& base, double& other);
void sub(double& base, double& other);
void mul(double& base, double& other);
void div(double& base, double& other);
}
