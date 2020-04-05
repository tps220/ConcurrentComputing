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
#include "Benchmark.hpp"

#define DEFAULT_EXPERIMENT_RANGE 200000
#define INITIAL_SIZE 20000
#define DEFAULT_SIZE (100000 / ENTRY_WIDTH)
Cuckoo<int, int> *store = NULL;
#define BUFFERS_PER_CLIENT 4
#define DEFAULT_BUFFER_SIZE 64

struct RDMAConnection {
  infinity::core::Context *context;
  infinity::queues::QueuePair *qp;
  RDMAConnection(infinity::core::Context *context, infinity::queues::QueuePair *qp) : context(context), qp(qp) {}
  RDMAConnection() : context(NULL), qp(NULL) {}
};

RDMAConnection** connections = NULL;

void populate_store(unsigned int seed) {
  for (int i = 0; i < INITIAL_SIZE; i++) {
    Node<int, int> element(rand_range_re(&seed, DEFAULT_EXPERIMENT_RANGE), rand_range_re(&seed, DEFAULT_EXPERIMENT_RANGE));
    store -> insert(element);
  }
}

void connection_handler(const int id) {
	RDMAConnection* thread_connection_set = connections[id];
	infinity::core::Context *context = thread_connection_set[0].context;
	while (1) {
		infinity::core::receive_element_t receiveElement;
		while (!context->receive(&receiveElement));
		Node<int, int>* elements = (Node<int, int>*)receiveElement.buffer -> getData();
		for (int i = 0; i < receiveElement.bytesWritten / sizeof(Node<int, int>); i++) {
			store -> insert(elements[i]);
		}
		context->postReceiveBuffer(receiveElement.buffer);
		//handle operation
	}
}

int main(int argc, char** argv) {
  const int PORT_NUMBER = 8011; //get from environment setup in future
  GlobalView environment = Parser::getEnvironment();
  store = new Cuckoo<int, int>(DEFAULT_SIZE);
	std::vector<std::thread> threads;
  populate_store(9); //only for testing on a single server, otherwise inconsistente state across nodes
	
	std::cout << "REPLICATED HASH MAP" << std::endl << "STORAGE_SIZE: " << store -> getStorageSize() << std::endl;
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
	infinity::queues::QueuePairFactory *qpFactory = new infinity::queues::QueuePairFactory(context);
	qpFactory -> bindToPort(PORT_NUMBER);
	for (int i = 0; i < environment.nodes.size(); i++) {
		connections[i] = new RDMAConnection[environment.clientsPerServer];
		for (int j = 0; j < environment.clientsPerServer; j++) {
			printf("Creating buffers to receive a message for node %d\n", j);
			for (int iterator = 0; iterator < BUFFERS_PER_CLIENT; iterator++) {
				infinity::memory::Buffer *bufferToReceive = new infinity::memory::Buffer(context, DEFAULT_BUFFER_SIZE);
				context->postReceiveBuffer(bufferToReceive);
			}
			infinity::queues::QueuePair *qp;
			printf("Setting up connection (blocking)\n");
			qp = qpFactory->acceptIncomingConnection(bufferToken, sizeof(infinity::memory::RegionToken)); //automatically restiers queue pair to a map bound to the context

		 	RDMAConnection connection(context, qp);
		 	connections[i][j] = connection;
		 	std::cout << "Established connection " << i << "," << j << std::endl;
		}
		threads.push_back(std::thread(connection_handler, i));
	}
	std::cout << "Finished opening connections" << std::endl;
	while(1) {}
}