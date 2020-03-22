#pragma once
#include <iostream>

template <typename K, typename V>
struct Node {
  K key;
  V val;
  Node(K key, V val) : key(key), val(val) {}
};