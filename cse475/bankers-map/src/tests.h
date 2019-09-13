// CSE 375/475 Assignment #1
// Fall 2019

// Description: This file declares a function that should be able to use the
// configuration information to drive tests that evaluate the correctness and
// performance of the map type you create.

// Run a bunch of tests to ensure the data structure works correctly
#include "simplemap.h"
#include "locks.h"
#include "config_t.h"
#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <thread>

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
  struct timespec start, finish;
  simplemap_t<int, float>* accounts;
};

void test_driver(config_t& cfg);
void do_work(thread_data_t* thread);
bool deposit(simplemap_t<int, float>* map, thread_data_t* thread);
float balance(simplemap_t<int, float>* map, unsigned int range);
