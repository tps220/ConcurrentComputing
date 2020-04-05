#pragma once
#include <iostream>
#include <vector>
#include "ServerNode.hpp"
#include "Result.hpp"
#include "Parser.hpp"
#include <infinity/core/Context.h>
#include <infinity/queues/QueuePairFactory.h>
#include <infinity/queues/QueuePair.h>
#include <infinity/memory/Buffer.h>
#include <infinity/memory/RegionToken.h>
#include <infinity/requests/RequestToken.h>

#define ENTRY_WIDTH 8
#define SIZE (1000000 / ENTRY_WIDTH)
#define LOCK_WIDTH 8

enum FUNCTION {
  PRIMARY = 1,
  SECONDARY = 2
};

struct Node {
  int key;
  int val;
  Node(int key, int val) : key(key), val(val) {}
  Node() : key(0), val(0) {}
};

struct RDMAConnection {
  infinity::core::Context *context;
  infinity::queues::QueuePair *qp;
  infinity::memory::RegionToken *remoteBufferToken;
  infinity::memory::Atomic *atomicOperation;
  RDMAConnection(infinity::core::Context *ctx, infinity::queues::QueuePair *qp, infinity::memory::RegionToken *remoteBufferToken) : context(ctx), qp(qp), remoteBufferToken(remoteBufferToken), atomicOperation(new infinity::memory::Atomic(ctx)) {}
};

class Remote {
private:
  GlobalView environment;
  unsigned int numNodes;
  std::vector<std::vector<RDMAConnection>> connections;
  //pthread_mutex_t* locks;
  unsigned int hash(int key, FUNCTION func);
public:
  Remote(GlobalView environment);
  ~Remote();
  //void testConnections();
  RESULT get(int key, int threadId);
  RESULT insert(std::pair<int, int> element, int threadId);
  void release(const int key, const int threadId, std::vector<int> node_ownership_set);
  std::vector<int> prepareMessage(const int key, const int threadId);
  RESULT insert(std::vector<std::pair<int, int>> elements, int threadId);
  void awaitMaster();
};