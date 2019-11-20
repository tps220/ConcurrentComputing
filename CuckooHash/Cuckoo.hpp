#pragma once

#include <iostream>
#include <stdlib.h>
#include <vector>
#include <queue>
#include <unordered_set>
using namespace std;

static unsigned long x=123456789, y=362436069, z=521288629;
inline unsigned long xorshf96(void) {
  unsigned long t;
  x ^= x << 16;
  x ^= x >> 5;
  x ^= x << 1;

  t = x;
  x = y;
  y = z;
  z = t ^ x ^ y;

  return z;
}

#define LOCK_TYPE pthread_rwlock_t
#define WRITE_LOCK(x) pthread_rwlock_wrlock(&(x))
#define READ_LOCK(x) pthread_rwlock_rdlock(&(x))
#define UNLOCK(x) pthread_rwlock_unlock(&(x))
#define WIDTH 6
#define MAX_SEARCH_DEPTH 7
#define EMPTY 0

template <typename T>
class Table {
public:
  volatile T** elements;

  static bool isOccupied(volatile T val) {
    return val != EMPTY;
  }

  static bool isOccupied(volatile T* bucket, int slot) {
    return isOccupied(bucket[slot]);
  }

  Table(unsigned int size) {
    this -> elements = new volatile T*[size];
    for (int i = 0; i < size; i++) {
      elements[i] = new volatile T[WIDTH]();
    }
  }

  bool find(T val, int idx) {
    volatile T* bucket = this -> elements[idx];
    for (int i = 0; i < WIDTH; i++) {
      if (bucket[i] == val) {
        return true;
      }
    }
    return false;
  }

  int findSlot(T val, int idx) {
    volatile T* bucket = this -> elements[idx];
    for (int i = 0; i < WIDTH; i++) {
      if (bucket[i] == val) {
        return i;
      }
    }
    return -1;
  }
  
  int isAvailable(int idx) {
    volatile T* bucket = this -> elements[idx];
    for (int i = 0; i < WIDTH; i++) {
      if (!isOccupied(bucket[i])) {
        return i;
      }
    }
    return -1;
  }

  bool isAvailable(int idx, int slot) {
    return !isOccupied(this -> elements[idx][slot]);
  }

  T getElement(int idx, int slot) {
    return this -> elements[idx][slot];
  }

  bool isOccupied(int idx, int slot) {
    return isOccupied(this -> elements[idx][slot]);
  }
};

enum STATUS {
  RESIZE,
  RETRY,
  DUPLICATE,
  SUCCESS
};

struct Result {
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

template <typename T>
class Cuckoo {
public:
  bool insert(T val) {
    const int idx1 = hash(val, 1), idx2 = hash(val, 2);
    while (true) {
      std::cout << "START INSERT" << std::endl;
      READ_LOCK(global_lock);
      std::cout << "START OP" << std::endl;
      Result result = insert(val, idx1, idx2);
      std::cout << "GOT A RESULT" << std::endl;
      //Possbile failures
      if (result.status == DUPLICATE) {
        UNLOCK(global_lock);
        return false;
      }
      else if (result.status == RESIZE) {
        std::cout << "RESIZE OP" << std::endl;
        UNLOCK(global_lock);
        resize();
        continue;
      }
      else if (result.status == RETRY) {
        std::cout << "RETRY" << std::endl;
        UNLOCK(global_lock);
        continue;
      }

      if (result.table == 1) {
        this -> table1 -> elements[result.index][result.slot] = val;
      }
      else {
        this -> table2 -> elements[result.index][result.slot] = val;
      }
      unlock_two(idx1, idx2);
      UNLOCK(global_lock);
      std::cout << "RETURNED INSERT" << std::endl;
      return true;
    }
  }

  bool remove(T val) {
    std::cout << "REMOVE OP" << std::endl;
    READ_LOCK(global_lock);
    const int idx1 = hash(val, 1), idx2 = hash(val, 2);
    this -> lock_two(idx1, idx2);
    int slot;
    if ((slot = table1 -> findSlot(val, idx1)) != -1) {
      table1 -> elements[idx1][slot] = EMPTY;
    }
    else if ((slot = table2 -> findSlot(val, idx2)) != -1) {
      table2 -> elements[idx2][slot] = EMPTY;
    }
    this -> unlock_two(idx1, idx2);
    UNLOCK(global_lock);
    return slot != -1;
  }

  bool contains(T val) {
    READ_LOCK(global_lock);
    const int idx1 = hash(val, 1), idx2 = hash(val, 2);
    lock_two(idx1, idx2);
    bool retval = table1 -> find(val, idx1) || table2 -> find(val, idx2);
    unlock_two(idx1, idx2);
    UNLOCK(global_lock);
    return retval;
  }

  int size() {
    int size = 0;
    for (int i = 0; i < this -> buckets; i++) {
      for (int j = 0; j < WIDTH; j++) {
        size += 1 & table1 -> isOccupied(i, j);
        size += 1 & table2 -> isOccupied(i, j);
      }
    }
    return size;
  }

  Cuckoo(unsigned int buckets) : buckets(buckets) {
    this -> locks = new LOCK_TYPE[buckets];
    for (int i = 0; i < buckets; i++) {
      pthread_rwlock_init(&this -> locks[i], NULL);
    }
    pthread_rwlock_init(&this -> global_lock, NULL);

    this -> table1 = new Table<T>(buckets);
    this -> table2 = new Table<T>(buckets);
  }

private:
  volatile unsigned int buckets;
  Table<T>* table1;
  Table<T>* table2;
  LOCK_TYPE* locks;
  LOCK_TYPE global_lock;

  int hash(T val, int function) {
    switch (function) { 
      case 1: 
        return val % this -> buckets; 
      case 2: 
        return (val / this -> buckets) % this -> buckets;
    }
  }

  void lock_two(int idx1, int idx2) {
    std::cout << "LOCKING TWO " << idx1 << " " << idx2;
    if (idx1 < idx2) {
      WRITE_LOCK(this -> locks[idx1]);
      WRITE_LOCK(this -> locks[idx2]);
    }
    else if (idx1 > idx2) {
      WRITE_LOCK(this -> locks[idx2]);
      WRITE_LOCK(this -> locks[idx1]);
    }
    else {
      WRITE_LOCK(this -> locks[idx1]);
    }
  }

  void lock_two_read_only(int idx1, int idx2) {
    if (idx1 < idx2) {
      READ_LOCK(this -> locks[idx1]);
      READ_LOCK(this -> locks[idx2]);
    }
    else if (idx1 > idx2) {
      READ_LOCK(this -> locks[idx2]);
      READ_LOCK(this -> locks[idx1]);
    }
    else {
      READ_LOCK(this -> locks[idx1]);
    }
  }

  void unlock_one(int idx) {
      std::cout << "UNLOCK ONE " << idx << std::endl;
    UNLOCK(this -> locks[idx]);
  }

  void unlock_two(int idx1, int idx2) {
    std::cout << "UNLOCKING 2 " << idx1 << " " << idx2 << std::endl;
    if (idx1 != idx2) {
      UNLOCK(this -> locks[idx1]);
      UNLOCK(this -> locks[idx2]);
    }
    else {
      UNLOCK(this -> locks[idx1]);
    }
  }

  void unlock_three(int idx1, int idx2, int idx3) {
    std::cout << "UNLOCKING 3 " << idx1 << " " << idx2 << " " << idx3 << std::endl;
    if (idx1 != idx2 && idx2 != idx3 && idx1 != idx3) {
      UNLOCK(this -> locks[idx1]);
      UNLOCK(this -> locks[idx2]);
      UNLOCK(this -> locks[idx3]);
    }
    else if (idx1 == idx2 && idx2 == idx3) {
      UNLOCK(this -> locks[idx1]);
    }
    else if (idx1 == idx2) {
      UNLOCK(this -> locks[idx1]);
      UNLOCK(this -> locks[idx3]);
    }
    else {
      UNLOCK(this -> locks[idx1]);
      UNLOCK(this -> locks[idx2]);
    }
  }

  void lock_three(int idx1, int idx2, int idx3) {
    std::cout << "LOCKING 3 " << idx1 << " " << idx2 << " " << idx3 << std::endl;
    if (idx1 == idx2 && idx2 == idx3) {
        WRITE_LOCK(this -> locks[idx1]);
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
    if (idx1 < idx2) {
      if (idx1 < idx3) {
        WRITE_LOCK(this -> locks[idx1]);
        if (idx2 < idx3) {
          WRITE_LOCK(this -> locks[idx2]);
          WRITE_LOCK(this -> locks[idx3]);
        }
        else {
          WRITE_LOCK(this -> locks[idx3]);
          WRITE_LOCK(this -> locks[idx2]);
        }
      }
      else {
        WRITE_LOCK(this -> locks[idx3]);
        WRITE_LOCK(this -> locks[idx1]);
        WRITE_LOCK(this -> locks[idx2]);
      }
    }
    else {
      if (idx2 < idx3) {
        WRITE_LOCK(this -> locks[idx2]);
        if (idx1 < idx3) {
          WRITE_LOCK(this -> locks[idx1]);
          WRITE_LOCK(this -> locks[idx3]);
        }
        else {
          WRITE_LOCK(this -> locks[idx3]);
          WRITE_LOCK(this -> locks[idx1]);
        }
      }
      else {
        WRITE_LOCK(this -> locks[idx3]);
        WRITE_LOCK(this -> locks[idx2]);
        WRITE_LOCK(this -> locks[idx1]);
      }
    }
  }

  Point nextPoint(Point point, int slot) {
    if (point.table == 1) {
      return Point(
        2, //table
        hash(
          this -> table1 -> getElement(point.index, slot), //get element from table1 and rehash
          2
        ), //alternate index
        point.pathcode * WIDTH + slot, //next pathcode
        point.depth + 1 //increse depth by 1
      );
    }
    return Point(
      1, //table
      hash(
        this -> table2 -> getElement(point.index, slot), //get element from table2 and rehash
        1
      ),
      point.pathcode * WIDTH + slot, //next pathcode
      point.depth + 1 //increase depth by 1
    );
  }

  Point search(const int idx1, const int idx2) {
    std::queue<Point> q;
    std::unordered_set<T> paths;
    q.push(Point(1, idx1, 0, 0));
    q.push(Point(2, idx2, 1, 0));
    while (!q.empty()) {
      Point point = q.front();
      q.pop();
      volatile T* bucket = this -> getBucket(point); 
      T val = bucket[point.pathcode % WIDTH];

      std::cout << point.index << " " << point.pathcode << " " << point.table << " " << val << std::endl;
      if (paths.find(val) != paths.end()) {
          std::cout << "CYCLE" << std::endl;
          return Point(-1, -1, -1, -1);
      }
      paths.insert(val);

      const int start_slot = xorshf96() % WIDTH;
      for (int i = 0; i < WIDTH; i++) {
        const int slot = (start_slot + i) % WIDTH;
        if (!Table<T>::isOccupied(bucket, slot)) {
          point.pathcode = point.pathcode * WIDTH + slot;
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

  volatile T* getBucket(Point point) {
    if (point.table == 1) {
      return this -> table1 -> elements[point.index];
    }
    return this -> table2 -> elements[point.index];
  }

  Result insert(T val, const int idx1, const int idx2) {
    int slot;
    this -> lock_two(idx1, idx2);
    if (this -> table1 -> find(val, idx1) || this -> table2 -> find(val, idx2)) {
      unlock_two(idx1, idx2);
      return { DUPLICATE, 0, 0, 0 };
    }
    if ((slot = this -> table1 -> isAvailable(idx1)) != -1) {
      return { SUCCESS, 1, idx1, slot };
    }
    if ((slot = this -> table2 -> isAvailable(idx2)) != -1) {
      return { SUCCESS, 2, idx2, slot };
    }
    this -> unlock_two(idx1, idx2);

    Point point = search(idx1, idx2);
    if (point.pathcode == -1) {
        std::cout << "COULD NOT FIND PATH" << std::endl;
        return { RESIZE, 0, 0, 0 };
    }

    Path path;
    for (int i = point.depth; i >= 0; i--) {
      path.slot[i] = point.pathcode % WIDTH;
      point.pathcode /= WIDTH;
    }

    int depth = -1;
    if (point.pathcode == 0) {
      path.index[0] = idx1;
      path.table[0] = 1;
      if (this -> table1 -> isAvailable(path.index[0], path.slot[0])) {
        depth = 0;
      }
    }
    else {
      path.index[0] = idx2;
      path.table[0] = 2;
      if (this -> table2 -> isAvailable(path.index[0], path.slot[0])) {
        depth = 0;
      }
    }

    for (int i = 1; i <= point.depth && depth == -1; i++) {
      if (path.table[i - 1] == 1) { //last one was table1, next point evalaute into table2
        path.table[i] = 2;
        path.index[i] = hash(this -> table1 -> getElement(path.index[i - 1], path.slot[i - 1]), 2);
        if (this -> table2 -> isAvailable(path.index[i], path.slot[i])) {
          depth = i;
        }
      }
      else { //last one was table2, next point evaluate into table1
        path.table[i] = 1;
        path.index[i] = hash(this -> table2 -> getElement(path.index[i - 1], path.slot[i - 1]), 1);
        if (this -> table1 -> isAvailable(path.index[i], path.slot[i])) {
          depth = i;
        }
      }
    }

    if (depth == -1) {
      depth = point.depth;
    }

    std::cerr << "DEPTH: " << depth << std::endl;
    for (int i = 0; i <= depth; i++) {
      std::cerr << "INDEX " << path.index[i] << " SLOT: " << path.slot[i] << std::endl;
    }

    if (depth == 0) {
      lock_two(idx1, idx2);
      if (path.table[0] == 1 && this -> table1 -> isAvailable(path.index[0], path.slot[0])) {
        return { SUCCESS, 1, path.index[0], path.slot[0] };
      }
      else if (path.table[0] == 2 && this -> table2 -> isAvailable(path.index[0], path.slot[0])) {
        return { SUCCESS, 2, path.index[0], path.slot[0] }; //need to determine which table it belongs to
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

      if (path.table[depth - 1] == 1) { //table1 from, table2 to
        if (table2 -> isOccupied(path.index[depth], path.slot[depth]) //if to is occupied, error
          || !table1 -> isOccupied(path.index[depth - 1], path.slot[depth - 1])  //if from is not occupied, error
          || this -> hash(table1 -> getElement(path.index[depth - 1], path.slot[depth - 1]), 2) != path.index[depth]) { //else if element has changed and hash does not equal
          
          std::cout << "FAILED HERE 1: " << (int)table2 -> isOccupied(path.index[depth], path.slot[depth]) << " " << (int)!table1 -> isOccupied(path.index[depth - 1], path.slot[depth - 1]) << " " << (int)(this -> hash(table1 -> getElement(path.index[depth - 1], path.slot[depth - 1]), 2) != path.index[depth]) << std::endl;
          if (depth == 1) {
            unlock_three(idx1, idx2, path.index[depth]);
          }
          else {
            unlock_two(path.index[depth], path.index[depth - 1]);
          }
          return { RETRY, 0, 0, 0 };
        }
      
        this -> table2 -> elements[path.index[depth]][path.slot[depth]] = this -> table1 -> elements[path.index[depth - 1]][path.slot[depth - 1]];
        this -> table1 -> elements[path.index[depth - 1]][path.slot[depth - 1]] = EMPTY;
      }
      else { //table2 from, table1 to
        if (table1 -> isOccupied(path.index[depth], path.slot[depth])
          || !table2 -> isOccupied(path.index[depth - 1], path.slot[depth - 1])
          || this -> hash(table2 -> getElement(path.index[depth - 1], path.slot[depth -1]), 1) != path.index[depth]) {

          std::cout << "FAILED HERE 2: " << table1 -> isOccupied(path.index[depth], path.slot[depth]) << " " << !table2 -> isOccupied(path.index[depth - 1], path.slot[depth - 1]) << " " << (this -> hash(table2 -> getElement(path.index[depth - 1], path.slot[depth - 1]), 1) != path.index[depth]) << std::endl;
          if (depth == 1) {
            unlock_three(idx1, idx2, path.index[depth]);
          }
          else {
            unlock_two(path.index[depth], path.index[depth - 1]);
          }
          return { RETRY, 0, 0, 0 };
        }

        this -> table1 -> elements[path.index[depth]][path.slot[depth]] = this -> table2 -> elements[path.index[depth - 1]][path.slot[depth - 1]];
        this -> table2 -> elements[path.index[depth - 1]][path.slot[depth - 1]] = EMPTY;
      }


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

  void resize() {
    const unsigned int old_capacity = this -> buckets;
    WRITE_LOCK(global_lock);
    if (old_capacity != this -> buckets) {
      UNLOCK(global_lock);
      return;
    }

    this -> buckets = this -> buckets * 4;

    Table<T> *t1 = new Table<T>(this -> buckets);
    Table<T> *t2 = new Table<T>(this -> buckets);

    for (int i = 0; i < this -> buckets / 4; i++) {
      for (int j = 0; j < WIDTH; j++) {
        T val = this -> table1 -> elements[i][j];
        if (Table<T>::isOccupied(val)) {
          int idx = this -> hash(val, 1);
          int slot = t1 -> isAvailable(idx);
          t1 -> elements[idx][slot] = val;
        }

        val = this -> table2 -> elements[i][j];
        if (Table<T>::isOccupied(val)) {
          int idx = this -> hash(val, 2);
          int slot = t2 -> isAvailable(idx);
          t2 -> elements[idx][slot] = val;
        }
      }
    }

    LOCK_TYPE* new_locks = new LOCK_TYPE[this -> buckets];
    for (int i = 0; i < this -> buckets; i++) {
      pthread_rwlock_init(&new_locks[i], NULL);
    }
    this -> locks = new_locks;
    this -> table1 = t1;
    this -> table2 = t2;
    UNLOCK(global_lock);
  }
};
