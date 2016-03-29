#pragma once

namespace Operations
{
enum Type
{
	INCREMENT, DECREMENT, FLIP_SIGN, UNSUPPORTED
};

void increment(double& val);
void decrement(double& val);
void flipSign(double& val);
}
