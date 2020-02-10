#pragma once
#include <iostream>
#include <vector>
#include "ServerNode.hpp"
#include "Result.hpp"
#include "Message.hpp"

class Remote {
private:
  std::vector<ServerNode> nodes;
  std::vector<int> connections;
  unsigned int hash(int key);
  unsigned int getNode(int key);
  void sendMessage(const int fd, Message message);
  RESULT awaitResponse(const int fd);
public:
  Remote(std::vector<ServerNode> nodes);
  ~Remote();
  RESULT get(int key);
  RESULT insert(std::pair<int, int>);
  void awaitMaster();
};