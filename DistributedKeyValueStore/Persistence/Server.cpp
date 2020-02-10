#include <iostream>
#include <vector>
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
#include <thread>
#include <vector>
#include "Connection.hpp"
#include "Message.hpp"
#include "ConcurrentHashMap.hpp"

#define PORT 9000
#define THREAD_NUM 3
ConcurrentHashMap<int, int> *store = NULL;

void send_response(const int socketid, RESULT result) {
  char response[BUFFER_SIZE];
  size_t length = sprintf(response, "%d\n", result);
  Connection::stream_data(socketid, response, length);
}

void handleMessage(const int socketid) {
  Buffer* protocol = Connection::socket_get_line(socketid);
  if (!Message::verify(protocol)) {
    send_response(socketid, ABORT_FAILURE); //could not read
    return;
  }

  Message message = Message::deserialize(protocol);
  RESULT result;
  if (message.protocol == GET) {
    result = store -> contains(message.data.first);
  }
  else if (message.protocol == PUT) {
    result = store -> add(message.data);
  }
  //Invalid
  else {
    result = ABORT_FAILURE;
  }
  delete protocol;

  send_response(socketid, result);
}

void connection_handler(int connectionfd) {
  while (1) {
    handleMessage(connectionfd);
  }
  if (close(connectionfd) < 0) {
    Connection::die("Error in thread close(): ", strerror(errno));
  }
}

void poll_connections(int listenfd, void (*service_function)(int)) {
  std::vector<std::thread> threads;
  while(1) {
    /* block until we get a connection */
    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(sockaddr_in));
    socklen_t clientlen = sizeof(clientaddr);
    int connfd;
    if((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen)) < 0) {
      Connection::die("Error in accept(): ", strerror(errno));
    }
    /* print some info about the connection */
    struct hostent *hp;
    hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hp == NULL) {
      fprintf(stderr, "DNS error in gethostbyaddr() %d\n", h_errno);
      exit(0);
    }
    char *haddrp = inet_ntoa(clientaddr.sin_addr);
    printf("server connected to %s (%s)\n", hp->h_name, haddrp);
    threads.push_back(std::thread(service_function, connfd));
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
}

int main(int argc, char **argv) {
  const int port = 9000;
/*
  std::vector<std::thread> threads;
  for (int i = 0; i < THREAD_NUM; i++) {
    threads.push_back(std::thread(connection_handler));
  }
*/
    
  /* open a socket, and start handling requests */
  fprintf(stderr, "Opening Server Socket\n");
  int fd = Connection::Server::open_server_socket(port);
  fprintf(stderr, "Synchronusly Polling for Connections\n");
  poll_connections(fd, connection_handler);

/*
  for (auto& thread : threads){
    thread.join();
  }
*/
  if (close(fd) < 0) {
    Connection::die("Error in close(): ", strerror(errno));
  }
  exit(0);
}