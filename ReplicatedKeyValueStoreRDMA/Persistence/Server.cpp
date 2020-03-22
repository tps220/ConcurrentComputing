#include <iostream>
#include <stdlib.h>
//#include "Cuckoo.hpp"
//#include "Parser.hpp"
#include <infinity/core/Context.h>
#include <infinity/queues/QueuePairFactory.h>
#include <infinity/queues/QueuePair.h>
#include <infinity/memory/Buffer.h>
#include <infinity/memory/RegionToken.h>
#include <infinity/requests/RequestToken.h>

#define DEFAULT_SIZE 100000 / ENTRY_WIDTH
//Cuckoo<int, int> *store = NULL;

int main(int argc, char** argv) {
  const int PORT_NUMBER = 8011; //get from environment setup in future
  //GlobalView environment = Parser::getEnvironment();
  infinity::core::Context *context = new infinity::core::Context();
  infinity::queues::QueuePairFactory *qpFactory = new  infinity::queues::QueuePairFactory(context);
  infinity::queues::QueuePair *qp;

	printf("Creating buffers to read from and write to\n");
	infinity::memory::Buffer *bufferToReadWrite = new infinity::memory::Buffer(context, 128 * sizeof(char));
	infinity::memory::RegionToken *bufferToken = bufferToReadWrite->createRegionToken();

	printf("Creating buffers to receive a message\n");
	infinity::memory::Buffer *bufferToReceive = new infinity::memory::Buffer(context, 128 * sizeof(char));
	context->postReceiveBuffer(bufferToReceive);

	printf("Setting up connection (blocking)\n");
	qpFactory->bindToPort(PORT_NUMBER);
	qp = qpFactory->acceptIncomingConnection(bufferToken, sizeof(infinity::memory::RegionToken));

	printf("Waiting for message (blocking)\n");
	infinity::core::receive_element_t receiveElement;
	while(!context->receive(&receiveElement));

	printf("Message received\n");
	delete bufferToReadWrite;
	delete bufferToReceive;
  delete qp;
	delete qpFactory;
	delete context;

  //store = new Cuckoo<int, int>(DEFAULT_SIZE);
}