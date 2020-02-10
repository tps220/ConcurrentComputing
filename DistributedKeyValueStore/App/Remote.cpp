#include "Remote.hpp"
#include "Connection.hpp"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "Message.hpp"

Remote::Remote(std::vector<Node> nodes) {
  for (int i = 0; i < nodes.size(); i++) {
    int fd = Connection::Client::connect_to_server(nodes[i].server, nodes[i].port);
    this -> connections.push_back(fd);
  }
}

Remote::~Remote() {
  for (int fd : this -> connections) {
  	int rc;
	  if((rc = close(fd)) < 0) {
		  Connection::die("Close error: ", strerror(errno));
	  }
	  exit(0);
  }
}

unsigned int Remote::hash(int key) {
  return key % this -> nodes.size();
}

unsigned int Remote::getNode(int key) {
  return this -> connections[this -> hash(key)];
}

void Remote::sendMessage(const int fd, Message message) {
  char protocol[4];
  if (message.protocol == MESSAGE_TYPE::GET) {
    sprintf(protocol, "GET");
  }
  else {
    sprintf(protocol, "PUT");
  }
  protocol[3] = '\0';

  char response[BUFFER_SIZE];
  size_t length = sprintf(response, "%s %d,%d\n", protocol, message.data.first, message.data.second);
  Connection::stream_data(fd, response, length);
}

RESULT Remote::get(int key) {
  int fd = this -> getNode(key);
  Message message;
  message.data = { key, 0 };
  message.protocol = MESSAGE_TYPE::GET;
  
  sendMessage(fd, message);
  return awaitResponse(fd);
}

RESULT Remote::insert(std::pair<int, int> data) {
  int fd = this -> getNode(data.first);
  Message message;
  message.data = data;
  message.protocol = MESSAGE_TYPE::PUT;
  
  sendMessage(fd, message);
  return awaitResponse(fd);
}

RESULT Remote::awaitResponse(const int fd) {
  Buffer* response = Connection::socket_get_line(fd);
  if (response == NULL) {
    return RESULT::ABORT_FAILURE;
  }
  response -> data[response -> currentSize - 1] = '\0';
  return (RESULT)atoi(response -> data);
}
