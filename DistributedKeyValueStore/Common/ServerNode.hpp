#include <iostream>
#pragma once

struct ServerNode {
    int id;
    char* server;
    const int port;
    ServerNode(char* server, int port) : server(server), port(port) {}
};