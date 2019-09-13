#pragma once
#include <mutex>

#define LOCK(mutex) ((mutex).lock())
#define UNLOCK(mutex) ((mutex).unlock())
#define INTIALIZE_LOCK(mutex) //
#define LOCK_TYPE std::mutex

LOCK_TYPE* locks;
unsigned int lockSize;

void initializeLocks(unsigned int size) {
  locks = new LOCK_TYPE[size];
  lockSize = size;
}

void deleteLocks() {
  delete locks;
}

bool acquireLock(int idx) { 
  LOCK(locks[idx]); 
  return true;
}

bool releaseLock(int idx) {
  UNLOCK(locks[idx]);
  return true;
}

bool acquireLocks(int i, int j) {
  LOCK(locks[i]);
  LOCK(locks[j]);
  return true;
}

bool releaseLocks(int i, int j) {
  UNLOCK(locks[i]);
  UNLOCK(locks[j]);
  return true;
}

bool acquireAll_Reader() {
  for (int i = 0 ; i < lockSize; i++) {
    LOCK(locks[i]);
  }
  return true;
}

bool releaseAll_Reader() {
  for (int i = 0; i < lockSize; i++) {
    UNLOCK(locks[i]);
  }
  return true;
}