# distri-net
Framework for distributed computing. Uses RabbitMQ to communicate between nodes.

## Dependencies
* [POCO](http://pocoproject.org/)
* [AMQP-CPP](https://github.com/CopernicaMarketingSoftware/AMQP-CPP)

## Installation
Working instance of RabbitMQ is required. Also shovel plugin is necessary. It can be enabled by running:
```
rabbitmq-plugins enable rabbitmq_shovel
```

## Modules

### Manager
Sends commands for processing. Ensures that all parameters required for safe concurrent execution are set.

### Worker
Executes commands.