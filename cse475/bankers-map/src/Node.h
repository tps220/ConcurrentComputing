#include "utils.h"

template<class K, class V>
class Node {
public:
  K key;
  V value;
  Node<K, V>* next;
  LOCK_TYPE lock;

  int compare(const Node target) {
    if (this -> key > target -> key) {
      return 1;
    }
    else if (this -> key < target -> key) {
      return -1;
    }
    return 0;
  }

  Node(K key, V value) {
    this -> key = key;
    this -> value = value;
    this -> next = NULL;
    INITIALIZE_LOCK(this -> lock); //might be a no-op depending on type of lock
  }
}