#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

#include "CWorkerTasksParser.h"

int main(void)
{
    CWorkerTasksParser worker("localhost", 5672, AMQP::Login("dnet", "111111"), "/dnet");
    worker.run();
    return 0;
}
