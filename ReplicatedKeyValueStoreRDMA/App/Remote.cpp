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

inline unsigned int secondary_hash(unsigned int a) {
  a = (a+0x7ed55d16) + (a<<12);
  a = (a^0xc761c23c) ^ (a>>19);
  a = (a+0x165667b1) + (a<<5);
  a = (a+0xd3a2646c) ^ (a<<9);
  a = (a+0xfd7046c5) + (a<<3);
  a = (a^0xb55a4f09) ^ (a>>16);
  return a;
}

unsigned int Remote::hash(int key, FUNCTION func) {
  switch (func) { 
    case PRIMARY: 
      return key % SIZE; 
    case SECONDARY: 
      return secondary_hash(key) % SIZE;
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

  RDMAConnection connection = connections[targetId][threadId];
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

/*
RESULT Remote::insertTest(std::pair<int, int> element, int threadId) {
  const int key = element.first,
            node_set_size = this -> connections.size();
  const int startingId = fastrand() % this -> numNodes,
            index = hash(key, PRIMARY);
  
  const int read_offset = (SIZE * ENTRY_WIDTH + index) * sizeof(Node),
            read_length = LOCK_WIDTH;
  RESULT retval = RESULT::FALSE;
  
  for (int i = 0; i < node_set_size; i++) {
    const int targetId = (startingId + i) % node_set_size;
    RDMAConnection connection = connections[targetId][threadId];
    infinity::queues::QueuePair *qp = connection.qp;
    infinity::core::Context *context = connection.context;
    infinity::memory::RegionToken *remoteBufferToken = connection.remoteBufferToken;
    infinity::memory::Atomic *atomicOperation = connection.atomicOperation;
    infinity::requests::RequestToken requestToken(context);

    qp -> compareAndSwap(remoteBufferToken, atomicOperation, 0, 1, infinity::queues::OperationFlags(), read_offset, &requestToken); //CAS operation, remotely offset by location of lock, default operation falgs (fenced, signaled, inlined set to false)
    requestToken.waitUntilCompleted();
  }

  for (int i = 0; i < node_set_size; i++) {
    const int targetId = (startingId + i) % node_set_size;
    RDMAConnection connection = connections[targetId][threadId];
    infinity::queues::QueuePair *qp = connection.qp;
    infinity::core::Context *context = connection.context;
    infinity::memory::RegionToken *remoteBufferToken = connection.remoteBufferToken;
    infinity::memory::Atomic *atomicOperation = connection.atomicOperation;
    infinity::requests::RequestToken requestToken(context);

    qp -> compareAndSwap(remoteBufferToken, atomicOperation, 1, 0, infinity::queues::OperationFlags(), read_offset, &requestToken); //CAS operation, remotely offset by location of lock, default operation falgs (fenced, signaled, inlined set to false)
    requestToken.waitUntilCompleted();
  }
  return RESULT::TRUE;
}
*/

void Remote::release(const int key, const int threadId, std::vector<int> node_ownership_set) {
  const int index = hash(key, PRIMARY);
  const int read_offset = (SIZE * ENTRY_WIDTH + index) * sizeof(Node);

  for (int i = 0; i < node_ownership_set.size(); i++) {
    const int targetId = node_ownership_set[i];
    RDMAConnection connection = connections[targetId][threadId];
    infinity::queues::QueuePair *qp = connection.qp;
    infinity::core::Context *context = connection.context;
    infinity::memory::RegionToken *remoteBufferToken = connection.remoteBufferToken;
    infinity::memory::Atomic *atomicOperation = connection.atomicOperation;
    infinity::requests::RequestToken requestToken(context);

    qp -> compareAndSwap(remoteBufferToken, atomicOperation, 1, 0, infinity::queues::OperationFlags(), read_offset, &requestToken); //CAS operation, remotely offset by location of lock, default operation falgs (fenced, signaled, inlined set to false)
    requestToken.waitUntilCompleted(); //spray and then collect values might be a better approach, offset will be in scenarios of high contention when stopping early might help reduce unnecessary newtork calls
  }
}

std::vector<int> Remote::prepareMessage(const int key, const int threadId) {
  const int node_set_size = this -> connections.size(),
            startingId = fastrand() % this -> numNodes,
            index = hash(key, PRIMARY);
  
  const int read_offset = (SIZE * ENTRY_WIDTH + index) * sizeof(Node);
  std::vector<int> node_ownership_set;

  for (int i = 0; i < node_set_size; i++) {
    const int targetId = (startingId + i) % node_set_size;
    RDMAConnection connection = connections[targetId][threadId];
    infinity::queues::QueuePair *qp = connection.qp;
    infinity::core::Context *context = connection.context;
    infinity::memory::RegionToken *remoteBufferToken = connection.remoteBufferToken;
    infinity::memory::Atomic *atomicOperation = connection.atomicOperation;
    infinity::requests::RequestToken requestToken(context);

    qp -> compareAndSwap(remoteBufferToken, atomicOperation, 0, 1, infinity::queues::OperationFlags(), read_offset, &requestToken); //CAS operation, remotely offset by location of lock, default operation falgs (fenced, signaled, inlined set to false)
    requestToken.waitUntilCompleted(); //spray and then collect values might be a better approach, offset will be in scenarios of high contention when stopping early might help reduce unnecessary newtork calls

    if (atomicOperation -> getValue() == 1) { //previously 1
      release(key, threadId, node_ownership_set);
      return std::vector<int>();
    }
    node_ownership_set.push_back(targetId);
  }
}


RESULT Remote::insert(std::pair<int, int> element, int threadId) {
  Node node(element.first, element.second);
  std::vector<int> node_ownership_set = prepareMessage(node.key, threadId);
  if (node_ownership_set.size() == 0) {
    return RESULT::ABORT_FAILURE;
  }

  for (int i = 0; i < node_ownership_set.size(); i++) {
    const int targetId = node_ownership_set[i];
    RDMAConnection connection = connections[targetId][threadId];
    infinity::queues::QueuePair *qp = connection.qp;
    infinity::core::Context *context = connection.context;
    infinity::memory::RegionToken *remoteBufferToken = connection.remoteBufferToken;
    infinity::requests::RequestToken requestToken(context);

    Node insertion = node;
    infinity::memory::Buffer *buffer2Sided = new infinity::memory::Buffer(context, &insertion, 8);
    qp->send(buffer2Sided, &requestToken);
    requestToken.waitUntilCompleted();
  }

  release(node.key, threadId, node_ownership_set);
  return RESULT::TRUE;
}
