# DistriNET
Framework for distributed computing. Uses RabbitMQ to communicate between nodes.

[![Build Status](https://travis-ci.org/akademi4eg/distri-net.svg?branch=master)](https://travis-ci.org/akademi4eg/distri-net)

## Dependencies
* [POCO](http://pocoproject.org/)
* [AMQP-CPP](https://github.com/CopernicaMarketingSoftware/AMQP-CPP)

## Installation
Working instance of RabbitMQ is required (version 3.3.0 or newer). Also shovel plugin is necessary. It can be enabled by running:
```
rabbitmq-plugins enable rabbitmq_shovel
```

Note that to build sources you'll need g++ version 5 or higher.

## Modules

### Manager
### Overview
Sends commands to `batches_ops` exchange for processing. Ensures that all parameters required for safe concurrent execution are set.
Commands ordering achived using additional callback queues. Instructions in those queues wait until data necessary for them is ready. When all conditions are met, content of callback queue is shoveled into the main stream of instructions.
### Scripting
Trivial scripting is available. Script filename should be sent as a parameter to executable. For example:
```
./distnet_manager ../examples/ex01.txt
``` 
The syntax resambles assembler. Here is the example of possible script:
```
// Example 1.
ZEROS :a, 2
SET :b, -2, 3
INCREMENT :a
COPY :c, :a
ADD :a, :b
DIV :c, :a

// Expected output:
// a = [-1, 4]
// b = [-2, 3]
// c = [-1, 0.25]
```

### Worker
### Overview
Executes commands obtained from `batches_tasks` queue. After each instruction, modified data is stored to persistence storage. So far, persistence is implemented using local file system.

## Notes

### Shovels
Shovels are set using HTTP API for RabbitMQ, because there is no support of set_parameter call in AMQP-CPP library used.
Sometimes you have to use non-default port for HTTP API. In that case pass define with port during compilation process.
If your port of choice is 8080:
```
make CPPFLAGS=-DRABBITMQ_API_PORT=8080
```