#include "Parallelize.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <pthread.h>
#include <sys/time.h>

volatile int stop;

Thread_Data::Thread_Data(int id, thread_benchmark *benchmark) : 
  id(id), benchmark(benchmark) {}

ExperimentResult::ExperimentResult(int duration, std::vector<Thread_Data> data) : 
  duration(duration), data(data) {}

Parallelize::Parallelize(Cuckoo<int>* set, unsigned int range, unsigned int nb_threads) : 
  range(range), nb_threads(nb_threads), set(set) {}

ExperimentResult Parallelize::run(
  const val_t first,
  const int update,
  const int unit_tx,
  const int alternate,
  const int effective,
  const int seed,
  const int duration
) {
  barrier_t barrier;
  std::vector<std::thread> threads;
  std::vector<Thread_Data> thread_local_data;
  
  //1 Initialize random seed
  initializeRandomizer(seed);

  //2 Initialize Global Stop Condition
  stop = 0;

  //3 Initialize Barrier
  barrier_init(&barrier, this -> nb_threads + 1);
    
  //4 Spawn Threads and MAP
  for (int i = 0; i < this -> nb_threads; i++) {
    thread_local_data.push_back(
      Thread_Data(i, new thread_benchmark(first, this -> range, update, unit_tx, alternate, effective, &barrier))
    );
  }

  for (int i = 0; i < this -> nb_threads; i++) {
    threads.push_back(
      std::thread(execute_local_txs, &thread_local_data[i], set);
    );
  }

  //5 Record Time and Cross Barrier
  struct timeval start, end;
  struct timespec timeout;
  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;

  barrier_cross(&barrier);

  printf("STARTING...\n");
  gettimeofday(&start, NULL);
  nanosleep(&timeout, NULL);

  //6 End Test
  stop = 1;
  gettimeofday(&end, NULL);
  printf("STOPPING...\n");
  for (int i = 0; i < threads.size(); i++) {
    threads[i].join();
  }

  //8 Return results
  int test_duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
  return ExperimentResult(test_duration, thread_local_data);
}

void execute_local_txs(Thread_Data *thread, Cuckoo<int>* set) {
  int unext, last = -1; 
  val_t val = 0;
  thread_benchmark* data = thread -> benchmark;

  //Wait on barrier
  barrier_cross(data -> barrier);
  
  // Is the first op a write?
  unext = (rand_range_re(&data -> seed, 100) - 1 < data -> update);
    
  while (stop == 0) {
    if (unext) { //write tx
      if (last < 0) { //add        
        val = rand_range_re(&data -> seed, data -> range - 1) + 1;
        if (set -> insert(val)) {
          data -> nb_added++;
          last = val;
        }
        data -> nb_add++;
      } else { // remove  
        if (data ->alternate) { // alternate mode  
          if (set -> remove(last)) {
            data ->nb_removed++;
          }
          last = -1;  
        } else {
          val = rand_range_re(&data -> seed, data -> range - 1) + 1;
          if (set -> remove(val)) {
            data -> nb_removed++;
            last = -1;
          }
        }
        data -> nb_remove++;
      }
    } else { // read
      if (data -> alternate) {
        if (data -> update == 0) {
          if (last < 0) {
            val = data -> first;
            last = val;
          } else { // last >= 0
            val = rand_range_re(&data -> seed, data -> range - 1) + 1;
            last = -1;
          }
        } else { // update != 0
          if (last < 0) {
            val = rand_range_re(&data -> seed, data -> range - 1) + 1;
          } else {
            val = last;
          }
        }
      } else {
        val = rand_range_re(&data -> seed, data-> range - 1) + 1;
      }
      if (set -> contains(val)) {
        data -> nb_found++;
      }
      data -> nb_contains++;     
    }

    /* Is the next op an update? */
    if (data -> effective) { // a failed remove/add is a read-only tx
      unext = ((100 * (data -> nb_added + data -> nb_removed))
         < (data -> update * (data -> nb_add + data -> nb_remove + data -> nb_contains)));
    } else { // remove/add (even failed) is considered an update
      unext = (rand_range_re(&data -> seed, 100) - 1 < data -> update);
    }
  }
  return;
}