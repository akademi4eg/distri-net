#include <iostream>

#include "CWorkerTasksParser.h"
#define Log(x) (std::cout << x << std::endl)

int main(void)
{
    Log("Client version "+c_sProductVersion);
    CWorkerTasksParser worker("localhost", 5672, AMQP::Login("dnet", "111111"), "/dnet");
    worker.run();
    return 0;
}
