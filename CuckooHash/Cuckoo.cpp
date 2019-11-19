#include <iostream>
#include <stdlib.h>
#include "Cuckoo.hpp"
#include <queue>

template<typename T>
Cuckoo<T>::Cuckoo(unsigned int buckets) : buckets(buckets) {
  this -> locks = new LOCK_TYPE[buckets];
  for (int i = 0; i < buckets; i++) {
    pthread_rwlock_init(&this -> locks[i], NULL);
  }
  pthread_rwlock_init(&this -> global_lock, NULL);

  this -> table1(buckets);
  this -> table2(buckets);
}


template<typename T>
bool Cuckoo<T>::remove(T val) {
  const int idx1 = hash(val, 1), idx2 = hash(val, 2);
  this -> lock_two(idx1, idx2);
  int slot;
  if ((slot = table1 -> findIndex(val, idx1)) != -1) {
    table1 -> elements[idx1][slot] = EMPTY;
  }
  else if ((slot = table2 -> findIndex(val, idx2)) != -1) {
    table2 -> elements[idx2][slot] = EMPTY;
  }
  this -> unlock_two(idx1, idx2);
  return slot != -1;
}

template<typename T>
bool Cuckoo<T>::contains(T val) {
  const int idx1 = hash(val, 1), idx2 = hash(val, 2);
  lock_two(idx1, idx2);
  bool retval = table1 -> find(val, idx1) || table2 -> find(val, idx2);
  unlock_two(idx1, idx2);
  return retval;
}

template<typename T>
bool Table<T>::isOccupied(int idx, int slot) {
  return isOccupied(this -> elements[idx][slot]);
}

template <typename T>
int Table<T>::isAvailable(int idx) {
  T* bucket = this -> elements[idx];
  for (int i = 0; i < this -> width; i++) {
    if (!isOccupied(this -> buckets[i])) {
      return i;
    }
  }
  return -1;
}

template <typename T>
bool Table<T>::find(T val, int idx) {
  T* bucket = this -> elements[idx];
  for (int i = 0; i < this -> width; i++) {
    if (this -> bucket[i] == val) {
      return true;
    }
  }
  return false;
}

template <typename T>
int Table<T>::findSlot(T val, int idx) {
  T* bucket = this -> element[idx];
  for (int i = 0; i < this -> width; i++) {
    if (this -> bucket[i] == val) {
      return i;
    }
  }
  return -1;
}

template <typename T>
T Table<T>::getElement(int idx, int slot) {
  return this -> elements[idx][slot];
}

template <typename T>
int Cuckoo<T>::hash(T val, int function) {
  switch (function) { 
    case 1: 
      return val % this -> buckets; 
    case 2: 
      return (val / this -> buckets) % this -> buckets;
  }
}

template <typename T>
void Cuckoo<T>::lock_two(int idx1, int idx2) {
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

template <typename T>
void Cuckoo<T>::lock_three(int idx1, int idx2, int idx3) {
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

template <typename T>
void Cuckoo<T>::unlock_one(int idx) {
  UNLOCK(this -> locks[idx]);
}

template <typename T>
void Cuckoo<T>::unlock_three(int idx1, int idx2, int idx3) {
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

template <typename T>
void Cuckoo<T>::lock_two_read_only(int idx1, int idx2) {
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

template <typename T>
void Cuckoo<T>::unlock_two(int idx1, int idx2) {
  if (idx1 != idx2) {
    UNLOCK(this -> table1 -> locks[idx1]);
    UNLOCK(this -> table2 -> locks[idx2]);
  }
  else {
    UNLOCK(this -> table1 -> locks[idx1]);
  }
}

template <typename T>
bool Cuckoo<T>::insert(T val) {
  const int idx1 = hash(val, 1), idx2 = hash(val, 2);
  while (true) {
    Result result = insert(val);
    //Possbile failures
    if (result.status == DUPLICATE) {
      return false;
    }
    else if (result.status == RESIZE) {
      resize();
      continue;
    }
    else if (result.status == RETRY) {
      continue;
    }

    if (result.table == 1) {
      this -> table1 -> elements[result.index][result.slot] = val;
    }
    else {
      this -> table2 -> elements[result.index][result.slot] = val;
    }
    unlock_two(idx1, idx2);
    return true;
  }
}

template <typename T>
T* Cuckoo<T>::getBucket(Point point) {
  if (point.table == 1) {
    return this -> table1 -> elements[point.index];
  }
  return this -> table2 -> elements[point.index];
}

template <typename T>
Point Cuckoo<T>::nextPoint(Point point, int slot) {
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
    point.pathcode * WIDTH + slot, //next patchdoe
    point.depth + 1 //increase depth by 1
  );
}

template <typename T>
Point Cuckoo<T>::search(const int idx1, const int idx2) {
  std::queue<Point> q;
  q.push(Point(1, idx1, 0, 0));
  q.push(Point(2, idx2, 1, 0));
  while (!q.empty()) {
    Point point = q.front();
    q.pop();

    T* bucket = this -> getBucket(point); 
    const int start = point.pathcode % WIDTH;
    for (int i = 0; i < WIDTH; i++) {
      const int slot = (start + i) % WIDTH;
      if (!isOccupied(bucket[slot])) {
        point.pathcode = point.pathcode * WIDTH + slot;
        return point;
      }
      
      if (point.depth < MAX_SEARCH_DEPTH) {
        q.push(
          nextPoint(point, slot)
        );
      }
    }
  }

  return Point(-1, -1, -1, -1);
}

//Returns: { status, table, index, slot }
template <typename T>
Result Cuckoo<T>::insert(T val, const int idx1, const int idx2) {
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
  unlock_two(idx1, idx2);

  Point point = search(idx1, idx2);
  if (point.pathcode == -1) {
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
    unlock_two(idx1, idx2);
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
         || !this -> hash(table2 -> getElement(path.index[depth - 1], path.slot[depth -1]), 1) != path.index[depth]) {

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
      unlock_one(path.index[depth]);
    }
    else {
      unlock_two(path.index[depth], path.index[depth - 1]);
    }
    depth--;
  }

  return { SUCCESS, path.table[0], path.index[0], path.slot[0] };
}

template <typename T>
void Cuckoo<T>::resize() {
  const unsigned int old_capacity = this -> buckets;
  WRITE_LOCK(global_lock);
  if (old_capacity != this -> buckets) {
    UNLOCK(global_lock);
    return;
  }

  this -> buckets = this -> buckets * 4;
  Table<T> t1 = new Table<T>(this -> buckets);
  Table<T> t2 = new Table<T>(this -> buckets);

  for (int i = 0; i < this -> buckets; i++) {
    for (int j = 0; j < WIDTH; j++) {
      T val = this -> table1[i][j];
      if (isOccupied(val)) {
        int idx = this -> hash(val, 1);
        int slot = this -> table1 -> isAvailable(idx);
        t1[idx][slot] = val;
      }

      val = this -> table2[i][j];
      if (isOccupied(val)) {
        int idx = this -> hash(val, 2);
        int slot = this -> table1 -> isAvailable(idx);
        t2[idx][slot] = val;
      }
    }
  }

  this -> table1 = t1;
  this -> table2 = t2;
}

template <typename T>
int Cuckoo<T>::size() {
  int size = 0;
  for (int i = 0; i < this -> buckets; i++) {
    for (int j = 0; j < WIDTH; j++) {
      size += 1 & table1 -> isOccupied(i, j);
      size += 1 & table2 -> isOccupied(i, j);
    }
  }
  return size;
}