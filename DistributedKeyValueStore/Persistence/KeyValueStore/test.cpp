#include <iostream>
#include <vector>
#include "ConcurrentHashMap.hpp"

#define SIZE 10000

int main(int argc, char** argv) {
  ConcurrentHashMap<int, int>* map = new ConcurrentHashMap<int, int>(20);
  std::vector<std::pair<int, int> > values;
  for (int i = 0; i < SIZE; i++) {
    int key = rand();
    int val = rand();
    std::pair<int, int> record = std::make_pair(key, val);
    values.push_back(record);
    map -> add(record);
  }

  for (int i = 0; i < SIZE; i++) {
    if (!map -> contains(values[i].first)) {
      std::cerr << " Failed Insertion " << std::endl;
    }
  }

  for (int i = 0; i < SIZE; i++) {
    if (!map -> remove(values[i].first)) {
      std::cerr << " Failed Deletion " << std::endl;
    }
  }

  for (int i = 0; i < SIZE; i++) {
    if (map -> contains(values[i].first)) {
      std::cerr << " Failed Post Deletion " << std::endl;
    }
  }

  exit(0);
}