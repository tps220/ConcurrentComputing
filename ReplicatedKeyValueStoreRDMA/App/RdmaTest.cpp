#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cassert>

#include <infinity/core/Context.h>
#include <infinity/queues/QueuePairFactory.h>
#include <infinity/queues/QueuePair.h>
#include <infinity/memory/Buffer.h>
#include <infinity/memory/RegionToken.h>
#include <infinity/requests/RequestToken.h>

int main(int argc, char** argv) {
  const int PORT_NUMBER = 8011;
  const char* SERVER_IP = "192.0.0.1";

  infinity::core::Context *context = new infinity::core::Context();
	infinity::queues::QueuePairFactory *qpFactory = new infinity::queues::QueuePairFactory(context);
	infinity::queues::QueuePair *qp;

  printf("Connecting to remote node\n");
	qp = qpFactory->connectToRemoteHost(SERVER_IP, PORT_NUMBER);
	infinity::memory::RegionToken *remoteBufferToken = (infinity::memory::RegionToken *) qp->getUserData();

	printf("Creating buffers\n");
	infinity::memory::Buffer *buffer1Sided = new infinity::memory::Buffer(context, 128 * sizeof(char));
	infinity::memory::Buffer *buffer2Sided = new infinity::memory::Buffer(context, 128 * sizeof(char));

	printf("Reading content from remote buffer\n");
	infinity::requests::RequestToken requestToken(context);
	qp->read(buffer1Sided, remoteBufferToken, &requestToken);
	requestToken.waitUntilCompleted();

	printf("Writing content to remote buffer\n");
	qp->write(buffer1Sided, remoteBufferToken, &requestToken);
	requestToken.waitUntilCompleted();

	printf("Sending message to remote host\n");
	qp->send(buffer2Sided, &requestToken);
	requestToken.waitUntilCompleted();

	delete buffer1Sided;
	delete buffer2Sided;
  delete qp;
	delete qpFactory;
	delete context;
}