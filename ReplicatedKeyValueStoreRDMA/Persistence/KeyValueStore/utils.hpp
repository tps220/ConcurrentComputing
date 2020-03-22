#pragma once
#include <stdlib.h>
#include <iostream>
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

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

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

typedef struct barrier {
  pthread_cond_t complete;
  pthread_mutex_t mutex;
  int count;
  int crossing;
} barrier_t;

inline void barrier_init(barrier_t *b, int n) {
  pthread_cond_init(&b->complete, NULL);
  pthread_mutex_init(&b->mutex, NULL);
  b->count = n;
  b->crossing = 0;
}

inline void barrier_cross(barrier_t *b) {
  pthread_mutex_lock(&b->mutex);
  /* One more thread through */
  b->crossing++;
  /* If not all here, wait */
  if (b->crossing < b->count) {
    pthread_cond_wait(&b->complete, &b->mutex);
  } else {
    pthread_cond_broadcast(&b->complete);
    /* Reset for next time */
    b->crossing = 0;
  }
  pthread_mutex_unlock(&b->mutex);
}

struct thread_benchmark {
  val_t first;
  long range;
  int update;
  int unit_tx;
  int alternate;
  int effective;
  unsigned long nb_add;
  unsigned long nb_added;
  unsigned long nb_remove;
  unsigned long nb_removed;
  unsigned long nb_contains;
  unsigned long nb_found;
  unsigned long nb_abort;
  unsigned int seed;
  barrier_t* barrier;

  thread_benchmark(
    val_t first,
    long range,
    int update,
    int unit_tx,
    int alternate,
    int effective,
    barrier_t* barrier
  ) : 
    first(first),
    range(range),
    update(update),
    unit_tx(unit_tx),
    alternate(alternate),
    effective(effective),
    nb_add(0),
    nb_added(0),
    nb_remove(0),
    nb_removed(0),
    nb_contains(0),
    nb_found(0),
    nb_abort(0),
    seed(rand()),
    barrier(barrier) {}
};