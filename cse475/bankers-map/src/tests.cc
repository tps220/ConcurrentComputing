// CSE 375/475 Assignment #1
// Fall 2019
//
// Description: This file implements a function 'run_custom_tests' that should be able to use
// the configuration information to drive tests that evaluate the correctness
// and performance of the map_t object
#include "tests.h"
#include "string.h"

#define MAX_AMOUNT 100000
#define EXISTS(pair) (pair).second
#define GET_BALANCE(pair) (pair).first

void printer(int k, float v) {
	std::cout<<"<"<<k<<","<<v<<">"<< std::endl;
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

void run_custom_tests(config_t& cfg) {
    srand((int)time(0));

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
    memset(&data, 0, sizeof(thread_data_t) * cfg.threads);

    //Execution do_work
    for (int i = 0; i < cfg.threads; i++) {
      data[i].id = i;
      data[i].seed = rand();
      data[i].keyRange = cfg.key_max;
      data[i].iters = cfg.iters / cfg.threads;
      data[i].deposit = initialBalance / 10;
      data[i].accounts = accounts;

      threads[i] = std::thread(do_work, &data[i]);
    }

    for (int i = 0; i < cfg.threads; i++) {
      threads[i].join();
    }

    double totalTime = 0;
    int deposits = 0;
    int balances = 0;
    for (int i = 0; i < cfg.threads; i++){
      totalTime +=( data[i].finish.tv_sec - data[i].start.tv_sec )
                + ( data[i].finish.tv_nsec - data[i].start.tv_nsec ) / (1000000000L * 1.0);
      deposits += data[i].nb_deposits;
      balances += data[i].nb_balances;
    }
    std::cout << "FINAL BALANCE: " << (int)balance(accounts, cfg.key_max) << std::endl;
    std::cout << "Performance:" << std::endl
              << "Total time " << totalTime << std::endl
              << "Time per thread " << totalTime / cfg.threads << std:: endl
              << "Balances " << balances << " (" << balances * 1.0 / (balances + deposits) * 100 << "%)" << std::endl
              << "Deposits " << deposits << " (" << deposits * 1.0 / (balances + deposits) * 100 << "%)" << std::endl;
		// Step 7
		// Now configure your application to perform the same total amount
		// of iterations just executed, but all done by a single thread.
		// Measure the time to perform them and compare with the time
		// previously collected.
		// Which conclusion can you draw?
		// Which optimization can you do to the single-threaded execution in
		// order to improve its performance?
    //accounts -> apply(printer);

    //Cleanup
    deleteLocks();
    delete accounts;

		// Final step: Produce plot
        // I expect each submission to include a plot in which
        // the x-axis is the concurrent threads used {1;2;4;8}
        // the y-axis is the application execution t ime.
        // The performance at 1 thread must be the sequential
        // application without atomic execution

        // You might need the following function to print the entire map.
        // Attention if you use it while multiple threads are operating
      
}

void do_work(thread_data_t* thread) {
  simplemap_t<int, float>* accounts = thread -> accounts;
  const unsigned int iters = thread -> iters;

  clock_gettime(CLOCK_MONOTONIC, &thread -> start);
  for (int i = 0; i < iters; i++) {
    if (rand_integer(&thread -> seed, 100) >= 20) {
      bool result = deposit(accounts, thread);
      if (result) {
        thread -> nb_deposits++;
      }
      thread -> deposits++;
    } else {
      float result = balance(accounts, thread -> keyRange);
      if (result) {
        thread -> nb_balances++;
      }
      thread -> balances++;
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &thread -> finish);
}

float balance(simplemap_t<int, float>* map, unsigned int range) {
  acquireAll_Reader();
  float total = 0;
  for (int i = 0; i < range; i++) {
    total += GET_BALANCE(map -> lookup(i));
  }
  releaseAll_Reader();
  return total;
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

bool deposit(simplemap_t<int, float>* map, thread_data_t* thread) {
	const unsigned int keySource = rand_integer(&thread -> seed, thread -> keyRange - 1), 
                     keyTarget = rand_integer(&thread -> seed, thread -> keyRange - 1);
	const float        amount = thread -> deposit;
  const unsigned int sourceBucket = map -> hash(keySource);
  const unsigned int targetBucket = map -> hash(keyTarget);

  bool retval;
  if (sourceBucket < targetBucket) {
    acquireLocks(sourceBucket, targetBucket);
    retval = applyDeposit(map, keySource, keyTarget, amount);
    releaseLocks(sourceBucket, targetBucket);
  }
  else if (sourceBucket > targetBucket) {
    acquireLocks(targetBucket, sourceBucket);
    retval = applyDeposit(map, keySource, keyTarget, amount);
    releaseLocks(targetBucket, sourceBucket);
  }
  else {
    acquireLock(targetBucket);
    retval = applyDeposit(map, keySource, keyTarget, amount);
    releaseLock(targetBucket);
  }
  return retval;
}

void test_driver(config_t &cfg) {
	run_custom_tests(cfg);
}
