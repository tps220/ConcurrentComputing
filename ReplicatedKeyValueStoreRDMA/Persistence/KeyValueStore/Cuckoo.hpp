#pragma once

#include <iostream>
#include <stdlib.h>
#include <vector>
#include <queue>
#include <unordered_set>
#include "Result.hpp"
#include "Node.hpp"
using namespace std;

#define ENTRY_WIDTH 8
#define MAX_SEARCH_DEPTH 5
#define EMPTY 0

static unsigned int g_seed = 99;
inline int fastrand() { 
  g_seed = (214013 * g_seed + 2531011); 
  return (g_seed >> 16) & 0x7FFF; 
}

template <typename K, typename V>
class Table {
public:
  Node<K, V>* elements;

  static bool isOccupied(Node<K, V> element) {
    return element.key != EMPTY;
  }

  bool isOccupied(int idx, int slot) {
    return isOccupied(this -> elements[idx * ENTRY_WIDTH + slot]);
  }

  Table(unsigned int size) {
    this -> elements = new Node<K, V>[size * ENTRY_WIDTH + size]; //contiguous block of elements + CAS locks
  }

  Node<K, V>* getOffset(int idx) {
    return (this -> elements + idx * ENTRY_WIDTH);
  }

  bool find(Node<K, V> element, int idx) {
    return find(element.key, idx);
  }

  bool find(K key, int idx) {
    Node<K, V> *bucket = this -> getOffset(idx);
    for (int i = 0; i < ENTRY_WIDTH; i++) {
      if (bucket[i].key == key) {
        return true;
      }
    }
    return false;
  }

  int findSlot(Node<K, V> element, int idx) {
    return findSlot(element.key, idx);
  }

  int findSlot(K key, int idx) {
    Node<K, V> *bucket = this -> getOffset(idx);
    for (int i = 0; i < ENTRY_WIDTH; i++) {
      if (bucket[i].key == key) {
        return i;
      }
    }
    return -1;
  }
  
  int isAvailable(int idx) {
    Node<K, V> *bucket = this -> getOffset(idx);
    for (int i = 0; i < ENTRY_WIDTH; i++) {
      if (!isOccupied(bucket[i])) {
        return i;
      }
    }
    return -1;
  }

  bool isAvailable(int idx, int slot) {
    return !isOccupied(idx, slot);
  }

  Node<K, V> getElement(int idx, int slot) {
    return this -> elements[idx * ENTRY_WIDTH + slot];
  }

  void setElement(int idx, int slot, Node<K, V> element) {
    this -> elements[idx * ENTRY_WIDTH + slot] = element;
  }

  void setKey(int idx, int slot, K key) {
    this -> elements[idx * ENTRY_WIDTH + slot].key = key;
  }
};

enum STATUS {
  RESIZE,
  RETRY,
  DUPLICATE,
  SUCCESS
};

struct InsertResult {
  STATUS status;
  int table;
  int index;
  int slot;
};

struct Point {
  int table;
  int index;
  int pathcode;
  int depth;
  Point(int table, int index, int pathcode, int depth) : table(table), index(index), pathcode(pathcode), depth(depth) {}
};

struct Path {
  int index[MAX_SEARCH_DEPTH];
  int slot[MAX_SEARCH_DEPTH];
  int table[MAX_SEARCH_DEPTH];
};

template <typename K, typename V>
class Cuckoo {
public:
  RESULT insert(Node<K, V> element) {
    const int idx1 = hash(element.key, 1), idx2 = hash(element.key, 2);
    while (true) {
      InsertResult result = insert(element, idx1, idx2);
      //Possbile failures
      if (result.status == RESIZE) {
        return RESULT::ABORT_FAILURE;
        continue;
      }
      else if (result.status == RETRY) {
        continue;
      }
      this -> table -> setElement(result.index, result.slot, element);
      unlock_two(idx1, idx2);
      return result.status == DUPLICATE ? RESULT::DUP : RESULT::TRUE;
    }
  }

  RESULT remove(K key) {
    const int idx1 = hash(key, 1), idx2 = hash(key, 2);
    this -> lock_two(idx1, idx2);
    int slot;
    if ((slot = this -> table -> findSlot(key, idx1)) != -1) {
      this -> table -> setKey(idx1, slot, EMPTY);
    }
    else if ((slot = this -> table -> findSlot(key, idx2)) != -1) {
      this -> table -> setKey(idx2, slot, EMPTY);
    }
    this -> unlock_two(idx1, idx2);
    return slot != -1 ? RESULT::TRUE : RESULT::FALSE;
  }

  /* Stale, incorrect reads may occurr. Only here for testing purposes */
  RESULT contains(K key) {
    const int idx1 = hash(key, 1), idx2 = hash(key, 2);
    bool retval = this -> table -> find(key, idx1) || this -> table -> find(key, idx2);
    return retval ? RESULT::TRUE : RESULT::FALSE;
  }

  int size() {
    int size = 0;
    for (int i = 0; i < this -> buckets; i++) {
      for (int j = 0; j < ENTRY_WIDTH; j++) {
        size += 1 & this -> table -> isOccupied(i, j);
      }
    }
    return size;
  }

  void* getRawData() {
    return (void*)this -> table -> elements;
  }

  unsigned int getStorageSize() {
    return this -> buckets * (ENTRY_WIDTH + 1) * sizeof(Node<K, V>);
  }

  Cuckoo(unsigned int buckets) : buckets(buckets) {
    this -> locks = new pthread_spinlock_t[buckets];
    for (int i = 0; i < buckets; i++) {
      pthread_spin_init(&this -> locks[i], 0);
    }
    this -> table = new Table<K, V>(buckets);
  }

private:
  volatile unsigned int buckets;
  Table<K, V>* table;
  pthread_spinlock_t* locks;

  int hash(K key, int function) {
    switch (function) { 
      case 1: 
        return key % this -> buckets; 
      case 2: 
        return (key / this -> buckets) % this -> buckets;
    }
  }

  void lock_two(int idx1, int idx2) {
    if (idx1 < idx2) {
      pthread_spin_lock(&this -> locks[idx1]);
      pthread_spin_lock(&this -> locks[idx2]);
    }
    else if (idx1 > idx2) {
      pthread_spin_lock(&this -> locks[idx2]);
      pthread_spin_lock(&this -> locks[idx1]);
    }
    else {
      pthread_spin_lock(&this -> locks[idx1]);
    }
  }

  void lock_two_read_only(int idx1, int idx2) {
    if (idx1 < idx2) {
      pthread_spin_lock(&this -> locks[idx1]);
      pthread_spin_lock(&this -> locks[idx2]);
    }
    else if (idx1 > idx2) {
      pthread_spin_lock(&this -> locks[idx2]);
      pthread_spin_lock(&this -> locks[idx1]);
    }
    else {
      pthread_spin_lock(&this -> locks[idx1]);
    }
  }

  void unlock_one(int idx) {
    pthread_spin_unlock(&this -> locks[idx]);
  }

  void unlock_two(int idx1, int idx2) {
    if (idx1 != idx2) {
      pthread_spin_unlock(&this -> locks[idx1]);
      pthread_spin_unlock(&this -> locks[idx2]);
    }
    else {
      pthread_spin_unlock(&this -> locks[idx1]);
    }
  }

  void unlock_three(int idx1, int idx2, int idx3) {
    if (idx1 != idx2 && idx2 != idx3 && idx1 != idx3) {
      pthread_spin_unlock(&this -> locks[idx1]);
      pthread_spin_unlock(&this -> locks[idx2]);
      pthread_spin_unlock(&this -> locks[idx3]);
    }
    else if (idx1 == idx2 && idx2 == idx3) {
      pthread_spin_unlock(&this -> locks[idx1]);
    }
    else if (idx1 == idx2) {
      pthread_spin_unlock(&this -> locks[idx1]);
      pthread_spin_unlock(&this -> locks[idx3]);
    }
    else {
      pthread_spin_unlock(&this -> locks[idx1]);
      pthread_spin_unlock(&this -> locks[idx2]);
    }
  }

  void lock_three(int idx1, int idx2, int idx3) {
    if (idx1 == idx2 && idx2 == idx3) {
        pthread_spin_lock(&this -> locks[idx1]);
        return;
    }
    else if (idx1 == idx2) {
        lock_two(idx1, idx3);
        return;
    }
    else if (idx2 == idx3) {
        lock_two(idx1, idx2);
        return;
    }
    else if (idx1 == idx3) {
        lock_two(idx1, idx2);
        return;
    }
    if (idx1 < idx2) {
      if (idx1 < idx3) {
        pthread_spin_lock(&this -> locks[idx1]);
        if (idx2 < idx3) {
          pthread_spin_lock(&this -> locks[idx2]);
          pthread_spin_lock(&this -> locks[idx3]);
        }
        else {
          pthread_spin_lock(&this -> locks[idx3]);
          pthread_spin_lock(&this -> locks[idx2]);
        }
      }
      else {
        pthread_spin_lock(&this -> locks[idx3]);
        pthread_spin_lock(&this -> locks[idx1]);
        pthread_spin_lock(&this -> locks[idx2]);
      }
    }
    else {
      if (idx2 < idx3) {
        pthread_spin_lock(&this -> locks[idx2]);
        if (idx1 < idx3) {
          pthread_spin_lock(&this -> locks[idx1]);
          pthread_spin_lock(&this -> locks[idx3]);
        }
        else {
          pthread_spin_lock(&this -> locks[idx3]);
          pthread_spin_lock(&this -> locks[idx1]);
        }
      }
      else {
        pthread_spin_lock(&this -> locks[idx3]);
        pthread_spin_lock(&this -> locks[idx2]);
        pthread_spin_lock(&this -> locks[idx1]);
      }
    }
  }

  Point nextPoint(Point point, int slot) {
    if (point.table == 1) {
      return Point(
        2, //table
        hash(
          this -> table -> getElement(point.index, slot).key, //get element from table1 and rehash
          2
        ), //alternate index
        point.pathcode * ENTRY_WIDTH + slot, //next pathcode
        point.depth + 1 //increse depth by 1
      );
    }
    return Point(
      1, //table
      hash(
        this -> table -> getElement(point.index, slot).key, //get element from table2 and rehash
        1
      ),
      point.pathcode * ENTRY_WIDTH + slot, //next pathcode
      point.depth + 1 //increase depth by 1
    );
  }

  Point search(const int idx1, const int idx2) {
    std::queue<Point> q;
    std::unordered_set<K> paths;
    q.push(Point(1, idx1, 0, 0));
    q.push(Point(2, idx2, 1, 0));
    while (!q.empty()) {
      Point point = q.front();
      q.pop();
      Node<K, V> element = table -> getElement(point.index, point.pathcode % ENTRY_WIDTH);

      if (paths.find(element.key) != paths.end()) { //in a loop
          return Point(-1, -1, -1, -1);
      }
      paths.insert(element.key);

      const int start_slot = fastrand() % ENTRY_WIDTH;
      for (int i = 0; i < ENTRY_WIDTH; i++) {
        const int slot = (start_slot + i) % ENTRY_WIDTH;
        if (!table -> isOccupied(point.index, slot)) {
          point.pathcode = point.pathcode * ENTRY_WIDTH + slot;
          return point;
        }
      
        if (point.depth < MAX_SEARCH_DEPTH - 1) {
          q.push(
            nextPoint(point, slot)
          );
        }
      }
    }
    return Point(-1, -1, -1, -1);
  }

  InsertResult insert(Node<K, V> element, const int idx1, const int idx2) {
    int slot;
    this -> lock_two(idx1, idx2);
    if ((slot = this -> table -> findSlot(element, idx1)) != -1) {
      return { DUPLICATE, 1, idx1, slot };
    }
    if ((slot = this -> table -> findSlot(element, idx2)) != -1) {
      return { DUPLICATE, 2, idx2, slot };
    }
    if ((slot = this -> table -> isAvailable(idx1)) != -1) {
      return { SUCCESS, 1, idx1, slot };
    }
    if ((slot = this -> table -> isAvailable(idx2)) != -1) {
      return { SUCCESS, 2, idx2, slot };
    }
    this -> unlock_two(idx1, idx2);

    Point point = search(idx1, idx2);
    if (point.pathcode == -1) {
      return { RESIZE, 0, 0, 0 };
    }

    Path path;
    for (int i = point.depth; i >= 0; i--) {
      path.slot[i] = point.pathcode % ENTRY_WIDTH;
      point.pathcode /= ENTRY_WIDTH;
    }

    int depth = -1;
    if (point.pathcode == 0) {
      path.index[0] = idx1;
      path.table[0] = 1;
    }
    else {
      path.index[0] = idx2;
      path.table[0] = 2;
    }
    
    if (this -> table -> isAvailable(path.index[0], path.slot[0])) {
      depth = 0;
    }

    for (int i = 1; i <= point.depth && depth == -1; i++) {
      if (path.table[i - 1] == 1) { //last one was table1, next point evalaute into table2
        path.table[i] = 2;
        path.index[i] = hash(this -> table -> getElement(path.index[i - 1], path.slot[i - 1]).key, 2);
        if (this -> table -> isAvailable(path.index[i], path.slot[i])) {
          depth = i;
        }
      }
      else { //last one was table2, next point evaluate into table1
        path.table[i] = 1;
        path.index[i] = hash(this -> table -> getElement(path.index[i - 1], path.slot[i - 1]).key, 1);
        if (this -> table -> isAvailable(path.index[i], path.slot[i])) {
          depth = i;
        }
      }
    }

    if (depth == -1) {
      depth = point.depth;
    }

    if (depth == 0) {
      lock_two(idx1, idx2);
      if (this -> table -> isAvailable(path.index[0], path.slot[0])) {
        return { SUCCESS, path.table[0], path.index[0], path.slot[0] };
      }
      else {
        unlock_two(idx1, idx2);
        return { RETRY, 0, 0, 0 };
      }
    }

    while (depth > 0) {
      if (depth == 1) {
        lock_three(idx1, idx2, path.index[depth]);
      }
      else {
        lock_two(path.index[depth], path.index[depth - 1]);
      }

      if (table -> isOccupied(path.index[depth], path.slot[depth]) //if to is occupied, error
        || !table -> isOccupied(path.index[depth - 1], path.slot[depth - 1])  //if from is not occupied, error
        || this -> hash(table -> getElement(path.index[depth - 1], path.slot[depth - 1]).key, 2) != path.index[depth]) { //else if element has changed and hash does not equal
        
        if (depth == 1) {
          unlock_three(idx1, idx2, path.index[depth]);
        }
        else {
          unlock_two(path.index[depth], path.index[depth - 1]);
        }
        return { RETRY, 0, 0, 0 };
      }
      this -> table -> setElement(
        path.index[depth],
        path.slot[depth],
        this -> table -> getElement(
          path.index[depth - 1],
          path.slot[depth - 1]
        )
      );
      this -> table -> setKey(
        path.index[depth - 1],
        path.slot[depth - 1],
        EMPTY
      );

      if (depth == 1) {
        if (path.index[depth] != idx1 && path.index[depth] != idx2) {
          unlock_one(path.index[depth]);
        }
      }
      else {
        unlock_two(path.index[depth], path.index[depth - 1]);
      }
      depth--;
    }
    return { SUCCESS, path.table[0], path.index[0], path.slot[0] };
  }

  void resize() {}
};
