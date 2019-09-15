// CSE 375/475 Assignment #1
// Fall 2019
//
// Description: This file implements a function 'run_custom_tests' that should be able to use
// the configuration information to drive tests that evaluate the correctness
// and performance of the map_t object
#include "tests.h"

inline const unsigned int rand_integer(unsigned int *, long r);
inline bool applyDeposit(simplemap_t<int, float>*, int, int, float);

void run_custom_tests(config_t& cfg) {
    initializeSystem(cfg);

    //Initialize map
    simplemap_t<int, float>* accounts = new simplemap_t<int, float>(cfg.buckets);

    //Fill up zero-sum game of accounts to balance of 100000
    float initialBalance = MAX_AMOUNT / (1.0 * cfg.key_max);
    for (int i = 0; i < cfg.key_max; i++) {
      accounts -> insert(i, initialBalance);
    }

    //Initialize Lock / Thread / Data Pool
    initializeLocks(cfg.buckets);
    std::thread threads[cfg.threads];
    thread_data_t data[cfg.threads];

    //Execution do_work
    for (int i = 0; i < cfg.threads; i++) {
      data[i] = constructThreadData(
        i, //id
        rand(), //seed
        cfg.key_max, //keyRange
        cfg.iters / cfg.threads, //iterations per thread
        initialBalance / 10, //deposit amount
        accounts, //map structure
        MULTI_THREAD //locks enabled
      );
      threads[i] = std::thread(do_work, &data[i]);
    }

    for (int i = 0; i < cfg.threads; i++) {
      threads[i].join();
    }
    assert(((int)balance(accounts, cfg.key_max, true)) == MAX_AMOUNT);
    printMultithreaded(cfg, data);
		
    //Run single thread and record results
    thread_data_t datum = constructThreadData(
      0, 
      rand(), 
      cfg.key_max, 
      cfg.iters, 
      initialBalance / 10, 
      accounts, 
      LOCKFREE
    );
    do_work(&datum);
    printSinglethreaded(datum);

    //Cleanup
    deleteLocks();
    delete accounts;
}

void do_work(thread_data_t* thread) {
  simplemap_t<int, float>* accounts = thread -> accounts;
  const unsigned int iters = thread -> iters;
  if (!thread -> lockfree) {
    barrier_cross();
  }

  clock_gettime(CLOCK_MONOTONIC, &thread -> start);
  for (int i = 0; i < iters; i++) {
    if (rand_integer(&thread -> seed, 100) >= 20) {
      bool result = deposit(accounts, thread);
      if (result) {
        thread -> nb_deposits++;
      }
      thread -> deposits++;
    } else {
      float result = balance(accounts, thread -> keyRange, thread -> lockfree);
      if (result) {
        thread -> nb_balances++;
      }
      thread -> balances++;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &thread -> finish);
}

float balance(simplemap_t<int, float>* map, unsigned int range, bool lockfree) {
  if (!lockfree) {
    acquireAll_Reader();
  }
  float total = 0;
  for (int i = 0; i < range; i++) {
    total += GET_BALANCE(map -> lookup(i));
  }
  if (!lockfree) {
    releaseAll_Reader();
  }
  return total;
}

bool deposit(simplemap_t<int, float>* map, thread_data_t* thread) {
	const unsigned int keySource = rand_integer(&thread -> seed, thread -> keyRange - 1), 
                     keyTarget = rand_integer(&thread -> seed, thread -> keyRange - 1);
	const float        amount = thread -> deposit;
  const unsigned int sourceBucket = map -> hash(keySource);
  const unsigned int targetBucket = map -> hash(keyTarget);

  bool retval;
  if (thread -> lockfree) {
    retval = applyDeposit(map, keySource, keyTarget, amount);
  }
  else if (sourceBucket < targetBucket) {
    acquireWriteLocks(sourceBucket, targetBucket);
    retval = applyDeposit(map, keySource, keyTarget, amount);
    releaseWriteLocks(sourceBucket, targetBucket);
  }
  else if (sourceBucket > targetBucket) {
    acquireWriteLocks(targetBucket, sourceBucket);
    retval = applyDeposit(map, keySource, keyTarget, amount);
    releaseWriteLocks(targetBucket, sourceBucket);
  }
  else {
    acquireWriteLock(targetBucket);
    retval = applyDeposit(map, keySource, keyTarget, amount);
    releaseWriteLock(targetBucket);
  }
  return retval;
}

inline bool applyDeposit(simplemap_t<int, float>* map, int keySource, int keyTarget, float amount) {
  std::pair<float, bool> sourceAccount = map -> lookup(keySource);
  if (EXISTS(sourceAccount)) {
    map -> update(keySource, GET_BALANCE(sourceAccount) - amount);
    std::pair<float, bool> targetAccount = map -> lookup(keyTarget);
    if (EXISTS(targetAccount)) {
      map -> update(keyTarget, GET_BALANCE(targetAccount) + amount);
      return true;
    }
    //fallback
    map -> update(keySource, GET_BALANCE(sourceAccount));
  }
  return false;
}

/* 
 * Returns a pseudo-random value in [1;range).
 * Depending on the symbolic constant RAND_MAX>=32767 defined in stdlib.h,
 * the granularity of rand() could be lower-bounded by the 32767^th which might
 * be too high for given values of range and initial.
 * Thread-safe, re-entrant
*/
inline const unsigned int rand_integer(unsigned int *seed, long r) {
  unsigned int m = RAND_MAX;
  unsigned int d, v = 0;
  do {
    d = (m > r ? r : m);
    v += 1 + (int)(d * ((double)rand_r(seed)/((double)(m)+1.0)));
    r -= m;
  } while (r > 0);
  return v;
}

void test_driver(config_t &cfg) {
	run_custom_tests(cfg);
}
