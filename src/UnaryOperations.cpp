#include "Operations.h"

namespace Operations
{

void increment(double& val)
{
	val++;
}
void decrement(double& val)
{
	val--;
}
void flipSign(double& val)
{
	val = -val;
}

}
