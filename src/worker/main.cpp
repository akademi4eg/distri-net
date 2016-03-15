#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

#define Log(x) (std::cout << x << std::endl)
#include "SimplePocoHandler.h"

int DoSomething(int n)
{
    return n+1;
}

int main(void)
{
	Log("Connecting to Rabbit.");
    SimplePocoHandler handler("localhost", 5672);
    AMQP::Connection connection(&handler, AMQP::Login("dnet", "111111"), "/dnet");
    AMQP::Channel channel(&connection);
    channel.setQos(1);

    Log("Declaring queue.");
    channel.declareQueue("rpc_queue");
    channel.consume("").onReceived([&channel](const AMQP::Message &message,
            uint64_t deliveryTag,
            bool redelivered)
    {
        const auto body = message.message();
        Log(" [.] Message "<<body<<" received.");

        AMQP::Envelope env(std::to_string(DoSomething(std::stoi(body))));
        env.setCorrelationID(message.correlationID());

        channel.publish("", message.replyTo(), env);
        channel.ack(deliveryTag);
    });

    Log(" [x] Awaiting RPC requests");
    handler.loop();
    return 0;
}
