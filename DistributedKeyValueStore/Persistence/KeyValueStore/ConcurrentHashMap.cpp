#include "ConcurrentHashMap.hpp"

template <typename K, typename V>
unsigned int ConcurrentHashMap<K, V>::translate(K key) {
  return key % this -> buckets;
}

template <typename K, typename V>
RESULT ConcurrentHashMap<K, V>::contains(K key) {
  int bucket = this -> translate(key);
  return this -> map[bucket] -> contains(key);
}

template <typename K, typename V>
RESULT ConcurrentHashMap<K, V>::remove(K key) {
  int bucket = this -> translate(key);
  return this -> map[bucket] -> remove(key);
}

template <typename K, typename V>
RESULT ConcurrentHashMap<K, V>::add(std::pair<K, V> node) {
  int bucket = this -> translate(node.first);
  return this -> map[bucket] -> add(node.second);
}

template <typename K, typename V>
ConcurrentHashMap<K, V>::ConcurrentHashMap(unsigned int buckets) : buckets(buckets) {
  this -> map = new LazyList<K, V>*[buckets];
  for (int i = 0; i < this -> buckets; i++) {
    map[i] = new LazyList<K, V>();
  }
}