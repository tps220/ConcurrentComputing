#include <iostream>

struct Node {
    int id;
    char* server;
    const int port;
    Node(char* server, int port) : server(server), port(port) {}
};