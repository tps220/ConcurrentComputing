#include "Remote.hpp"
#include "Benchmark.hpp"
#include <iostream>
#include <stdlib.h>
#include <pthread.h>

static unsigned int g_seed = 99;
inline int fastrand() { 
  g_seed = (214013 * g_seed + 2531011); 
  return (g_seed >> 16) & 0x7FFF; 
}

inline RESULT findElement(const Node* elements, int key) {
  for (int i = 0; i < ENTRY_WIDTH; i++) {
    if (elements[i].key == key) {
      return RESULT::TRUE;
    }
  }
  return RESULT::FALSE;
}

//Sharing context as outlined for best performance with limited number of threads
//by https://www.mcs.anl.gov/~balaji/pubs/2018/icpads/icpads18.verbs_res_sharing.pdf 
Remote::Remote(GlobalView environment) : numNodes(environment.nodes.size()) {
  infinity::core::Context *context = new infinity::core::Context();
  for (ServerNode node : environment.nodes) {
    std::vector<RDMAConnection> thread_connections;
    for (int i = 0; i < environment.clientsPerServer; i++) {
	    infinity::queues::QueuePairFactory *qpFactory = new infinity::queues::QueuePairFactory(context);

      printf("Connecting to remote node %s %d\n", node.server, node.port);

	    infinity::queues::QueuePair *qp = qpFactory -> connectToRemoteHost(node.server, node.port);
	    infinity::memory::RegionToken *remoteBufferToken = (infinity::memory::RegionToken *) qp->getUserData();
      thread_connections.push_back(RDMAConnection(context, qp, remoteBufferToken));
    }
    connections.push_back(thread_connections);
  }

  /*
    this -> locks = new pthread_mutex_t[environment.clientsPerServer];
    for (int i = 0; i < environment.clientsPerServer; i++) {
      pthread_mutex_init(&this -> locks[i], NULL);
    }
  */
}

Remote::~Remote() {
  for (std::vector<RDMAConnection> thread_connections : this -> connections) {
    for (RDMAConnection connection : thread_connections) {
      delete connection.qp;
    }
  }
  delete this -> connections[0][0].context; //shared context
  /*
    for (int i = 0; i < environment.clientsPerServer; i++) {
      pthread_mutex_destroy(&this -> locks[i]);
    }
    delete this -> locks;
  */
}

unsigned int Remote::hash(int key, FUNCTION func) {
  switch (func) { 
    case PRIMARY: 
      return key % SIZE; 
    case SECONDARY: 
      return (key / SIZE) % SIZE;
  }
}

RESULT Remote::get(int key, int threadId) {
  const int targetId = fastrand() % this -> numNodes,
            index = hash(key, PRIMARY),
            secondary_index = hash(key, SECONDARY);
  
  const int read_offset = index * ENTRY_WIDTH * sizeof(Node),
            secondary_read_offset = secondary_index * ENTRY_WIDTH * sizeof(Node),
            read_length = ENTRY_WIDTH * sizeof(Node);
  RESULT retval = RESULT::FALSE;

  //pthread_mutex_lock(&this -> locks[targetId]);

  RDMAConnection connection = connections[threadId][targetId];
  infinity::queues::QueuePair *qp = connection.qp;
  infinity::core::Context *context = connection.context;
  infinity::memory::RegionToken *remoteBufferToken = connection.remoteBufferToken;
  infinity::memory::Buffer *buffer1Sided = new infinity::memory::Buffer(context, 8 * sizeof(Node));
  infinity::requests::RequestToken requestToken(context);

  qp -> read(buffer1Sided, 0, remoteBufferToken, read_offset, read_length, infinity::queues::OperationFlags(), &requestToken); //one sided read, locally ofset by 0, remotely offset by location of key, read length of 8 Nodes, default operation falgs (fenced, signaled, inlined set to false)
  requestToken.waitUntilCompleted();
  retval = findElement((Node*)buffer1Sided -> getData(), key);

  if (retval == RESULT::FALSE) {
    qp -> read(buffer1Sided, 0, remoteBufferToken, secondary_read_offset, read_length, infinity::queues::OperationFlags(), &requestToken); //one sided read, locally ofset by 0, remotely offset by location of key, read length of 8 Nodes, default operation falgs (fenced, signaled, inlined set to false)
	  requestToken.waitUntilCompleted();
    retval = findElement((Node*)buffer1Sided -> getData(), key);
  }
  
  //pthread_mutex_unlock(&this -> locks[targetId]);

  delete buffer1Sided;
  return retval;
}