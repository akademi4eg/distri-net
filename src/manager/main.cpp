#include <iostream>
#include <chrono>
#include <thread>

#define Log(x) (std::cout << x << std::endl)
#include "SimplePocoHandler.h"

int main(int argc, const char* argv[])
{
	const std::string correlation("1");
	const std::string responses("resp");

	Log("Connecting to Rabbit.");
	SimplePocoHandler handler("localhost", 5672);
	AMQP::Connection connection(&handler, AMQP::Login("dnet", "111111"), "/dnet");
	AMQP::Channel channel(&connection);
	channel.setQos(1);

	Log("Declaring responses queue.");
	channel.declareQueue(responses, AMQP::exclusive);

	auto receiveCallback = [&](const AMQP::Message &message,
			uint64_t deliveryTag,
			bool redelivered)
	{
		if(message.correlationID() == correlation)
		{
			Log(" [.] Got " + message.message());
		}
		channel.ack(deliveryTag);
		//handler.quit();
	};

	channel.consume(responses).onReceived(receiveCallback);

	AMQP::Envelope env("msg");
	env.setCorrelationID(correlation);
	env.setReplyTo(responses);
	channel.publish("batches_ops", "task", env);
	Log(" [x] Invalid request.");

	env = AMQP::Envelope("VERSION");
	env.setCorrelationID(correlation);
	env.setReplyTo(responses);
	channel.publish("batches_ops", "task", env);
	Log(" [x] Version request.");

	env = AMQP::Envelope("EXIT");
	env.setCorrelationID(correlation);
	env.setReplyTo(responses);
	channel.publish("batches_ops", "task", env);
	Log(" [x] Exit request.");

	handler.loop();
	return 0;
}
