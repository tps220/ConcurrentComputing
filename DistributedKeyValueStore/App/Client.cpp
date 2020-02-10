#include <iostream>
#include <thread>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include "Connection.hpp"
#include "Benchmark.hpp"
#include "Remote.hpp"
#include "Parser.hpp"

volatile int stop = 0;

void execute_remote_txs(Thread_Data *thread) {
  int unext, last = -1; 
  val_t val = 0, val_2 = 0;
  thread_benchmark* data = thread -> benchmark;

  Remote *remote = new Remote(thread -> benchmark -> nodes); //initiate connections
  remote -> awaitMaster();
  
  // Is the first op a write?
  unext = (rand_range_re(&data -> seed, 100) - 1 < data -> update);
    
  while (stop == 0) {
    if (unext) { //write tx
      /*
      bool insert = (rand_range_re(&data -> seed, data -> range) % 100) < 60;
      if (insert) { //add        
        val = rand_range_re(&data -> seed, data -> range - 1) + 1;
        if (remote -> insert(val)) {
          data -> nb_added++;
          last = val;
        }
        data -> nb_add++;
      } else { // remove  
        if (data ->alternate) { // alternate mode  
          if (remote -> remove(last)) {
            data ->nb_removed++;
          }
          last = -1;  
        } else {
          val = rand_range_re(&data -> seed, data -> range - 1) + 1;
          if (remote -> remove(val)) {
            data -> nb_removed++;
            last = -1;
          }
        }
        data -> nb_remove++;
      }
      */    

     //100% of updates are inserts
      val = rand_range_re(&data -> seed, data -> range - 1) + 1;
      val_2 = rand_range_re(&data -> seed, data -> range - 1) + 1;
      std::pair<int, int> keys = {val, val_2};
      RESULT retval = remote -> insert(keys);
      if (retval == RESULT::TRUE) {
        data -> nb_added++;
        last = val;
      }
      else if (retval == RESULT::ABORT_FAILURE) {
        data -> nb_aborted++;
      }
      data -> nb_add++;
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
      RESULT retval = remote -> get(val);
      if (retval == RESULT::TRUE) {
        data -> nb_found++;
      }
      else if (retval == RESULT::ABORT_FAILURE) {
        data -> nb_aborted++;
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

  delete remote; //end connections
  return;
}

ExperimentResult run(
  const val_t first,
  const int update,
  const int unit_tx,
  const int alternate,
  const int effective,
  const int seed,
  const int duration,
  const int range,
  GlobalView environment
) {
  std::vector<std::thread> threads;
  std::vector<Thread_Data> thread_local_data;
  
  //1 Initialize random seed
  initializeRandomizer(seed);

  //2 Initialize Global Stop Condition
  stop = 0;

  //4 Spawn Threads and MAP
  for (int i = 0; i < environment.clientsPerServer; i++) {
    thread_local_data.push_back(
      Thread_Data(i, new thread_benchmark(first, range, update, unit_tx, alternate, effective, environment.nodes))
    );
  }

  for (int i = 0; i < environment.clientsPerServer; i++) {
    threads.push_back(
      std::thread(execute_remote_txs, &thread_local_data[i])
    );
  }

  //5 Record Time and Cross Barrier
  struct timeval start, end;
  struct timespec timeout;
  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;

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

/*
 * main() - parse command line, open a socket, transfer a file
 */
int main(int argc, char **argv) {
	/* for getopt */
	long  opt;
	char* servers = NULL;
  int duration = DEFAULT_DURATION;
  int initial = DEFAULT_INITIAL;
  int nb_threads = 1;
  long range = DEFAULT_RANGE;
  int seed = DEFAULT_SEED;
  int update = DEFAULT_UPDATE;
  int unit_tx = DEFAULT_LOCKTYPE;
  int alternate = DEFAULT_ALTERNATE;
  int effective = DEFAULT_EFFECTIVE;
  unsigned long reads, effreads, updates, effupds, size = 0, aborted;

	/* parse the command-line options. */
	while((opt = getopt(argc, argv, "d:r:u:")) != -1) {
		switch(opt) {
      case 'd':
        duration = atoi(optarg);
        break;
      case 'r':
        range = atol(optarg);
        break;
      case 'u':
        update = atoi(optarg);
        break;
		}
	}

  GlobalView environment = Parser::getEnvironment();
  ExperimentResult result = run(
    0,
    update,
    unit_tx,
    alternate,
    effective,
    seed,
    duration,
    range,
    environment
  );
  
  std::vector<Thread_Data> snapshot = result.data;
  duration = result.duration;

  std::vector<thread_benchmark> data;
  for (int i = 0; i < snapshot.size(); i++) {
    data.push_back(*snapshot[i].benchmark);
  }

  reads = 0;
  effreads = 0;
  updates = 0;
  effupds = 0;
  aborted = 0;
  for (int i = 0; i < nb_threads; i++) {
    #ifdef PRINT_THREAD_LOCAL_DATA
        printf("Thread %d\n", i);
        printf("  #add        : %lu\n", data[i].nb_add);
        printf("    #added    : %lu\n", data[i].nb_added);
        printf("  #remove     : %lu\n", data[i].nb_remove);
        printf("    #removed  : %lu\n", data[i].nb_removed);
        printf("  #contains   : %lu\n", data[i].nb_contains);
        printf("  #found      : %lu\n", data[i].nb_found);
    #endif

    reads += data[i].nb_contains;
    effreads += data[i].nb_contains + 
      (data[i].nb_add - data[i].nb_added) + 
      (data[i].nb_remove - data[i].nb_removed); 
    updates += (data[i].nb_add + data[i].nb_remove);
    effupds += data[i].nb_removed + data[i].nb_added; 
    aborted += data[i].nb_aborted;
    //size += data[i].diff;
    size += data[i].nb_added - data[i].nb_removed;
  }

  //printf("Set size      : %d\n", remote -> size(), size);
  printf("Duration      : %d (ms)\n", duration);
  printf("#txs          : %lu (%f / s)\n", reads + updates, (reads + updates) * 1000.0 / duration);
  
  printf("#read txs     : ");
  if (effective) {
    printf("%lu (%f / s)\n", effreads, effreads * 1000.0 / duration);
    printf("  #contains   : %lu (%f / s)\n", reads, reads * 1000.0 / duration);
  } else {
    printf("%lu (%f / s)\n", reads, reads * 1000.0 / duration);
  }
  
  printf("#eff. upd rate: %f \n", 100.0 * effupds / (effupds + effreads));
  
  printf("#update txs   : ");
  if (effective) {
    printf("%lu (%f / s)\n", effupds, effupds * 1000.0 / duration);
    printf("  #upd trials : %lu (%f / s)\n", updates, updates * 1000.0 / duration);
  } else {
    printf("%lu (%f / s)\n", updates, updates * 1000.0 / duration);
  }
  printf("#aborts %d\n", aborted);

	exit(0);
}