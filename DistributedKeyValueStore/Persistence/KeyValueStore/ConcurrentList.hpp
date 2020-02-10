#pragma once
#include <iostream>
#include <pthread.h>
#include "Node.hpp"
#include "Result.hpp"

template <typename K, typename V>
class LazyList {
private:
  Node<K, V>* head;
  bool validateLink(Node<K, V>* prev, Node<K, V>* next);

public:
  LazyList();
  RESULT contains(K key);
  RESULT add(std::pair<K, V> val);
  RESULT remove(K key);
};
