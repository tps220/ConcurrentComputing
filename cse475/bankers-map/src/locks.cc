#pragma once
#include <pthread.h>

#define WRITE_LOCK(rwlock) pthread_rwlock_wrlock((rwlock))
#define READ_LOCK(rwlock) pthread_rwlock_rdlock((rwlock))
#define READ_UNLOCK(rwlock) pthread_rwlock_unlock((rwlock))
#define WRITE_UNLOCK(rwlock) pthread_rwlock_unlock((rwlock))
#define INTIALIZE_LOCK(rwlock) pthread_rwlock_init((rwlock), NULL)
#define LOCK_TYPE pthread_rwlock_t

LOCK_TYPE* locks;
unsigned int lockSize;

void initializeLocks(unsigned int size) {
  locks = new LOCK_TYPE[size];
  for (int i = 0; i < size; i++) {
    INTIALIZE_LOCK(locks[i]);
  }
  lockSize = size;
}

void deleteLocks() {
  delete locks;
}

bool acquireWriteLock(int idx) { 
  WRITE_LOCK(locks[idx]); 
  return true;
}

bool releaseWriteLock(int idx) {
  WRITE_UNLOCK(locks[idx]);
  return true;
}

bool acquireWriteLocks(int i, int j) {
  WRITE_LOCK(locks[i]);
  WRITE_LOCK(locks[j]);
  return true;
}

bool releaseWriteLocks(int i, int j) {
  WRITE_UNLOCK(locks[i]);
  WRITE_UNLOCK(locks[j]);
  return true;
}

bool acquireAll_Reader() {
  for (int i = 0 ; i < lockSize; i++) {
    READ_LOCK(locks[i]);
  }
  return true;
}

bool releaseAll_Reader() {
  for (int i = 0; i < lockSize; i++) {
    READ_UNLOCK(locks[i]);
  }
  return true;
}