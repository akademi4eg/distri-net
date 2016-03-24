#pragma once

namespace Operations
{
enum Type
{
	INCREMENT, DECREMENT, FLIP_SIGN
};

double increment(double val);
double decrement(double val);
double flipSign(double val);
}
