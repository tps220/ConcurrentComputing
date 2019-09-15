#pragma once
#include <pthread.h>
#include "config_t.h"
#include <stdlib.h>
#include "simplemap.h"

struct barrier {
  pthread_cond_t complete;
  pthread_mutex_t mutex;
  int count;
  int crossing;
} thread_barrier;

struct thread_data_t {
  int id;
  unsigned int seed;
  unsigned int keyRange;
  unsigned int iters;
  float deposit;
  unsigned long nb_deposits;
  unsigned long deposits;
  unsigned long nb_balances;
  unsigned long balances;
  unsigned long nb_aborts;
  simplemap_t<int, float>* accounts;
  bool lockfree;
  struct timespec start, finish;
};

void barrier_init(int n);
void barrier_cross();
void initializeSystem(config_t& cfg);

thread_data_t constructThreadData(
  int id, 
  unsigned int seed, 
  unsigned int keyRange, 
  unsigned int iters, 
  float deposit, 
  simplemap_t<int, float>* accounts,
  bool lockfree
);
void printMultithreaded(const config_t &cfg, const thread_data_t* data);
void printSinglethreaded(const thread_data_t datum);
