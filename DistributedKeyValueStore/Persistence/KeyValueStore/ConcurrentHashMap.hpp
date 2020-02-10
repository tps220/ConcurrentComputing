#pragma once
#include <iostream>
#include <pthread.h>
#include "Result.hpp"
#include "ConcurrentList.hpp"

template <typename K, typename V>
class ConcurrentHashMap {
private:
  unsigned int buckets;
  LazyList<K, V>** map;
  unsigned int translate(K key);
public:
  ConcurrentHashMap(unsigned int buckets);
  RESULT contains(K key);
  RESULT add(std::pair<K, V> node);
  RESULT remove(K key);
  int size();
};