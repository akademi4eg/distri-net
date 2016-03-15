#include <iostream>

#define Log(x) (std::cout << x << std::endl)
#include "SimplePocoHandler.h"

int main(int argc, const char* argv[])
{
    const std::string correlation("1");

    Log("Connecting to Rabbit.");
    SimplePocoHandler handler("localhost", 5672);
    AMQP::Connection connection(&handler, AMQP::Login("dnet", "111111"), "/dnet");
    AMQP::Channel channel(&connection);

    Log("Declaring queue.");
    AMQP::QueueCallback callback = [&](const std::string &name,
            int msgcount,
            int consumercount)
    {
        AMQP::Envelope env("30");
        env.setCorrelationID(correlation);
        env.setReplyTo(name);
        channel.publish("","rpc_queue",env);
        Log(" [x] Sending request.");

    };
    channel.declareQueue(AMQP::exclusive).onSuccess(callback);

    auto receiveCallback = [&](const AMQP::Message &message,
            uint64_t deliveryTag,
            bool redelivered)
    {
        if(message.correlationID() != correlation)
            return;

        Log(" [.] Got " + message.message());
        handler.quit();
    };

    channel.consume("", AMQP::noack).onReceived(receiveCallback);

    handler.loop();
    return 0;
}
