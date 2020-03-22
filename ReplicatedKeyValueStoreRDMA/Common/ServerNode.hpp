#pragma once
#include <iostream>

struct ServerNode {
    int id;
    char* server;
    const int port;
    ServerNode(char* server, int port) : server(server), port(port) {}
};