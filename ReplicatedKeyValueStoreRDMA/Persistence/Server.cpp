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

struct RDMAConnection {
  infinity::core::Context *context;
  infinity::queues::QueuePair *qp;
  RDMAConnection(infinity::core::Context *context, infinity::queues::QueuePair *qp) : context(context), qp(qp) {}
  RDMAConnection() : context(NULL), qp(NULL) {}
};

RDMAConnection** connections = NULL;

void populate_store() {
  for (int i = 0; i < INITIAL_SIZE; i++) {
    Node<int, int> element(fastrand() % DEFAULT_RANGE, fastrand() % DEFAULT_RANGE);
    store -> insert(element);
  }
}

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
	std::vector<std::thread> threads;
  populate_store(); //only for testing on a single server, otherwise inconsistente state across nodes
	
	//Sharing context and memory regions
	//as outlined for best performance with limited number of threads
	//by https://www.mcs.anl.gov/~balaji/pubs/2018/icpads/icpads18.verbs_res_sharing.pdf
	infinity::core::Context *context = new infinity::core::Context();
	infinity::memory::Buffer *bufferToReadWrite = new infinity::memory::Buffer(
		context, 
		store -> getRawData(), 
		store -> getStorageSize(),
		IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_ATOMIC | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
	);
	infinity::memory::RegionToken *bufferToken = bufferToReadWrite -> createRegionToken();
	connections = new RDMAConnection*[environment.nodes.size()];
	for (int i = 0; i < environment.nodes.size(); i++) {
		connections[i] = new RDMAConnection[environment.clientsPerServer];
		for (int j = 0; j < environment.clientsPerServer; j++) {
  		infinity::queues::QueuePairFactory *qpFactory = new infinity::queues::QueuePairFactory(context);
  		infinity::queues::QueuePair *qp;

			printf("Creating buffers to receive a message\n");
			infinity::memory::Buffer *bufferToReceive = new infinity::memory::Buffer(context, 128 * sizeof(char));
			context->postReceiveBuffer(bufferToReceive);

			printf("Setting up connection (blocking)\n");
			qpFactory -> bindToPort(PORT_NUMBER);
			qp = qpFactory->acceptIncomingConnection(bufferToken, sizeof(infinity::memory::RegionToken));

		 RDMAConnection connection(context, qp);
		 connections[i][j] = connection;
		 std::cout << "Established connection " << i << "," << j << std::endl;
		 //threads.push_back(std::thread(connection_handler, connection));
		}
	}
	std::cout << "Finished opening connections" << std::endl;
	while(1) {}
}