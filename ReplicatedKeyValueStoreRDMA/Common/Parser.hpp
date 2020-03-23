#pragma once
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string.h>
#include "ServerNode.hpp"

struct GlobalView {
  std::vector<ServerNode> nodes;
  int clientsPerServer;
};

inline std::vector<ServerNode> parseNodes(char* input) {
  int i = 0;
  std::vector<ServerNode> nodes;
  int window = 0;
  while (input[i] != '\0') {
    if (input[i] == ':') {
      input[i] = '\0';
      i++;
      int port = 0;
      while (input[i] >= '0' && input[i] <= '9') {
        port = port * 10 + input[i] - '0';
        i++;
      }
      nodes.push_back(ServerNode(input + window, port));
      window = i + 1;
    }
    i++;
  }
  return nodes;
}

class Parser {
public:
  static GlobalView getEnvironment() {
    GlobalView environment;

    FILE * fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("../Tests/environment", "r");
    if (fp == NULL) {
      exit(EXIT_FAILURE);
    }

    if ((read = getline(&line, &len, fp)) != -1) {
      char* servers = (char*)malloc(sizeof(char) * read + 1);
      strcpy(servers, line);
      servers[read] = '\0';
      environment.nodes = parseNodes(servers);
    }
    else {
      exit(0);
    }

    if ((read = getline(&line, &len, fp)) != -1) {
      environment.clientsPerServer = atoi(line);
    }
    else {
      exit(0);
    }
    fclose(fp);
    return environment;
  }
};