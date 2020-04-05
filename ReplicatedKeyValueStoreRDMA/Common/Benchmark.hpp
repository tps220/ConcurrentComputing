#pragma once
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <pthread.h>

#define val_t int
#define DEFAULT_DURATION                10000
#define DEFAULT_INITIAL                 256
#define DEFAULT_NB_THREADS              1
#define DEFAULT_RANGE                   0x7FFFFFFF
#define DEFAULT_SEED                    0
#define DEFAULT_UPDATE                  20
#define DEFAULT_LOCKTYPE                2
#define DEFAULT_ALTERNATE               0
#define DEFAULT_EFFECTIVE               1

inline long rand_range(long r) {
  int m = RAND_MAX;
  int d, v = 0;
 
  do {
    d = (m > r ? r : m);
    v += 1 + (int)(d * ((double)rand()/((double)(m)+1.0)));
    r -= m;
  } while (r > 0);
  return v;
}

/* Thread-safe, re-entrant version of rand_range(r) */
inline long rand_range_re(unsigned int *seed, long r) {
  int m = RAND_MAX;
  int d, v = 0;
 
  do {
    d = (m > r ? r : m);    
    v += 1 + (int)(d * ((double)rand_r(seed)/((double)(m)+1.0)));
    r -= m;
  } while (r > 0);
  return v;
}

inline void initializeRandomizer(const int seed) {
  if (seed == 0) {
    srand((int)time(NULL));
  }
  else {
    srand(seed);
  }
}

struct thread_benchmark {
  long range;
  int update;
  int alternate;
  int effective;
  unsigned long nb_add;
  unsigned long nb_added;
  unsigned long nb_remove;
  unsigned long nb_removed;
  unsigned long nb_contains;
  unsigned long nb_found;
  unsigned long nb_aborted;
  unsigned int seed;

  thread_benchmark(
    long range,
    int update,
    int alternate,
    int effective
  ) : 
    range(range),
    update(update),
    alternate(alternate),
    effective(effective),
    nb_add(0),
    nb_added(0),
    nb_remove(0),
    nb_removed(0),
    nb_contains(0),
    nb_found(0),
    nb_aborted(0),
    seed(rand())
  {}
};

class Thread_Data {
public:
  //Unique Identifier
  int id;

  //Synchrobench specific data
  thread_benchmark *benchmark;

  Thread_Data(int id, thread_benchmark *benchmark) : 
    id(id), benchmark(benchmark) {}
};

class ExperimentResult {
public:
  int duration;
  std::vector<Thread_Data> data;
  ExperimentResult(int duration, std::vector<Thread_Data> data) : 
    duration(duration), data(data) {}
};