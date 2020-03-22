#pragma once
#include <pthread.h>

#define LOCK_TYPE pthread_mutex_lock
#define LOCK(p) pthread_mutex_lock(&(p))
#define TRY_LOCK(p) pthread_mutex_trylock(&(p))
#define UNLOCK(p) pthread_mutex_unlock(&(p))
#define INIT_LOCK(p) pthread_mutex_init(&(p), NULL)
#define DESTRUCT_LOCK(p) 

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