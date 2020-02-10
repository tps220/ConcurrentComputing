#pragma once
#include <iostream>
#include <pthread.h>

template <typename K, typename V>
struct Node {
  K key;
  V val;
  bool marked;
  pthread_mutex_t* lock;
  Node* next;

  Node(K key, V val) : key(key), val(val), marked(false), next(NULL) {
    INIT_LOCK(this -> lock);
  }
};