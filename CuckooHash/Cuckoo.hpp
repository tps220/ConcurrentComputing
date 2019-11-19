#include <iostream>
#include <stdlib.h>
#include <vector>
using namespace std;

#define LOCK_TYPE pthread_rwlock_t
#define WRITE_LOCK(x) pthread_rwlock_wrlock((x))
#define READ_LOCK(x) pthread_rwlock_rdlock((x))
#define UNLOCK(x) pthread_rwlock_unlock((x))
#define WIDTH 6
#define MAX_SEARCH_DEPTH 10
#define EMPTY 0

template <typename T>
class Table {
  Table(unsigned int size);
  volatile T** elements;
  bool find(T val, int idx);
  int findSlot(T val, int idx);
  int isAvailable(int idx);
  T getElement(int idx, int slot);
  bool isOccupied(int idx, int slot);
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
  bool insert(T val);
  bool remove(T val);
  bool contains(T val);
  int size();
  Cuckoo(unsigned int buckets);
private:
  volatile unsigned int buckets;
  int hash(T val, int function);
  volatile Table<T> table1;
  volatile Table<T> table2;
  LOCK_TYPE* locks;
  LOCK_TYPE global_lock;
  void lock_two(int idx1, int idx2);
  void lock_two_read_only(int idx1, int idx2);
  void unlock_one(int idx);
  void unlock_two(int idx1, int idx2);
  void unlock_three(int idx1, int idx2, int idx3);
  void lock_three(int idx1, int idx2, int idx3);
  Point nextPoint(Point point, int slot);
  Point search(const int idx1, const int idx2);
  T* getBucket(Point point);
  Result insert(T val, const int idx1, const int idx2);
  void resize();
};