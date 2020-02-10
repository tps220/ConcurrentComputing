#include "ConcurrentList.hpp"
#include <climits>

//sample concurrent list template class
template class LazyList<int, int>;

template <typename K, typename V>
bool LazyList<K, V>::validateLink(Node<K, V>* prev, Node<K, V>* next) {
  return (volatile Node<K, V>*)(prev -> next) == next && !prev -> marked && !next -> marked;
}

template <typename K, typename V>
RESULT LazyList<K, V>::contains(K key) {
  Node<K, V>* runner = this -> head;
  while (runner -> key < key) {
    runner = runner -> next;
  }
  return (runner -> key == key && runner -> marked == false) ? RESULT::TRUE : RESULT::FALSE;
}

template <typename K, typename V>
RESULT LazyList<K, V>::remove(K key) {
  bool retry = true;
  while (retry) {
    Node<K, V> *pred = this -> head, *runner = this -> head -> next;
    while (runner -> key < key) {
      pred = runner;
      runner = runner -> next;
    }
    if (TRY_LOCK(pred -> lock)) {
      return RESULT::ABORT_FAILURE;
    }
    if (TRY_LOCK(runner -> lock)) {
      UNLOCK(pred -> lock);
      return RESULT::ABORT_FAILURE;
    }
    if (!validateLink(pred, runner)) {
      UNLOCK(pred -> lock);
      UNLOCK(runner -> lock);
      continue;
    }
    RESULT result = RESULT::FALSE;
    if (runner -> key == key) {
      runner -> marked = true;
      pred -> next = runner -> next;
      result = TRUE;
    }
    UNLOCK(pred -> lock);
    UNLOCK(runner -> lock);
    return result;
  }
}

template <typename K, typename V>
RESULT LazyList<K, V>::add(std::pair<K, V> val) {
  bool retry = true;
  const K target = val.first;
  while (retry) {
    Node<K, V> *pred = this -> head, *runner = this -> head -> next;
    while (runner -> key < target) {
      pred = runner;
      runner = runner -> next;
    }
    if (TRY_LOCK(pred -> lock)) {
      return RESULT::ABORT_FAILURE;
    }
    if (TRY_LOCK(runner -> lock)) {
      UNLOCK(pred -> lock);
      return RESULT::ABORT_FAILURE;
    }
    if (!validateLink(pred, runner)) {
      UNLOCK(pred -> lock);
      UNLOCK(runner -> lock);
      continue;
    }
    RESULT result = RESULT::FALSE;
    if (runner -> key != target) {
      Node<K, V>* insertion = new Node<K, V>(val.first, val.second);
      insertion -> next = runner;
      pred -> next = insertion;
      result = TRUE;
    }
    UNLOCK(pred -> lock);
    UNLOCK(runner -> lock);
    return result;
  }
}

template <typename K, typename V>
LazyList<K, V>::LazyList() {
  this -> head = new Node<K, V>(INT_MIN, INT_MIN);
  this -> head -> next = new Node<K, V>(INT_MAX, INT_MAX);
}

template <typename K, typename V>
int LazyList<K, V>::size() {
  int size = -2;
  Node<K, V>* runner = this -> head;
  while (runner != NULL) {
    size++;
    runner = runner -> next;
  }
  return size;
}