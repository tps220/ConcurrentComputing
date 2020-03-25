#include <iostream>
#include <stdlib.h>
#include "Cuckoo.hpp"
#include "Parser.hpp"
#include <infinity/core/Context.h>
#include <infinity/queues/QueuePairFactory.h>
#include <infinity/queues/QueuePair.h>
#include <infinity/memory/Buffer.h>
#include <infinity/memory/RegionToken.h>
#include <infinity/requests/RequestToken.h>
#include <thread>

#define DEFAULT_RANGE 200000
#define INITIAL_SIZE 10000
#define DEFAULT_SIZE 100000 / ENTRY_WIDTH
Cuckoo<int, int> *store = NULL;

void populate_store() {
  for (int i = 0; i < INITIAL_SIZE; i++) {
    Node<int, int> element(fastrand() % DEFAULT_RANGE, fastrand() % DEFAULT_RANGE);
    store -> insert(element);
  }
}

struct RDMAConnection {
  infinity::core::Context *context;
  infinity::queues::QueuePair *qp;
  RDMAConnection(infinity::core::Context *context, infinity::queues::QueuePair *qp) : context(context), qp(qp) {}
};

void connection_handler(RDMAConnection connection) {
	infinity::core::Context *context = connection.context;
	while (1) {
		infinity::core::receive_element_t receiveElement;
		while (!context->receive(&receiveElement));
		context->postReceiveBuffer(receiveElement.buffer);

		//handle operation
	}
}

int main(int argc, char** argv) {
  const int PORT_NUMBER = 8011; //get from environment setup in future
  GlobalView environment = Parser::getEnvironment();
  store = new Cuckoo<int, int>(DEFAULT_SIZE);
  populate_store(); //only for testing on a single server, otherwise inconsistente state across nodes
	
  std::vector<std::thread> threads;
  for (int i = 0; i < environment.nodes.size(); i++) {
    infinity::core::Context *context = new infinity::core::Context();
  	infinity::queues::QueuePairFactory *qpFactory = new  infinity::queues::QueuePairFactory(context);
  	infinity::queues::QueuePair *qp;

		printf("Creating buffers to read from and write to\n");
		infinity::memory::Buffer *bufferToReadWrite = new infinity::memory::Buffer(context, store -> getRawData(), store -> getStorageSize());
		infinity::memory::RegionToken *bufferToken = bufferToReadWrite->createRegionToken();

		printf("Creating buffers to receive a message\n");
		infinity::memory::Buffer *bufferToReceive = new infinity::memory::Buffer(context, 128 * sizeof(char));
		context->postReceiveBuffer(bufferToReceive);

		printf("Setting up connection (blocking)\n");
		qpFactory->bindToPort(PORT_NUMBER);
		qp = qpFactory->acceptIncomingConnection(bufferToken, sizeof(infinity::memory::RegionToken));

		RDMAConnection connection(context, qp);
		threads.push_back(std::thread(connection_handler, connection));
	}
	while(1) {}
}