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
#include "Synchronization.hpp"
#include "Result.hpp"

volatile int stop = 0;
barrier_t local_barrier; //need a local and global barrier in order to coordinate timing across and within nodes
Remote* remote = NULL;

#define READ_TXS true

#ifdef READ_WRITE_TXS
void execute_remote_txs(Thread_Data *thread) {
  int unext, last = -1; 
  val_t val = 0, val_2 = 0;
  thread_benchmark* data = thread -> benchmark;

  barrier_cross(&local_barrier);
  
  // Is the first op a write?
  unext = (rand_range_re(&data -> seed, 100) - 1 < data -> update);
    
  while (stop == 0) {
    if (unext) { // update
      if (last < 0) { // add
        val = rand_range_re(&data -> seed, data -> range);
        val_2 = rand_range_re(&data -> seed, data -> range - 1) + 1;
        std::pair<int, int> keys = { val, val_2 };
        RESULT retval = remote -> insert(keys);
        if (retval == RESULT::TRUE) {
          data -> nb_added++;
          last = val;
        }
        else if (retval == RESULT::ABORT_FAILURE) {
          data -> nb_aborted++;
        }
        data -> nb_add++;
      } else { // remove
        if (data -> alternate) { // alternate mode (default)
          RESULT retval = remote -> remove(last);
          if (retval == RESULT::TRUE) {
            data -> nb_removed++;
          }
          else if (retval == RESULT::ABORT_FAILURE) {
            data -> nb_aborted++;
          }
          last = -1;
        } else {
          /* Random computation only in non-alternated cases */
          val = rand_range_re(&data -> seed, data -> range);
          RESULT retval = remote -> remove(val);
          /* Remove one random value */
          if (retval == RESULT::TRUE) {
            data -> nb_removed++;
            /* Repeat until successful, to avoid size variations */
            last = -1;
          }
          else if (retval == RESULT::ABORT_FAILURE) {
            data -> nb_aborted++;
          }
        }
        data -> nb_remove++;
      }

    } else { // read
      val = rand_range_re(&data -> seed, data -> range);
      RESULT retval = remote -> get(val);
      if (retval == RESULT::TRUE) {
        data -> nb_found++;
      }
      else if (retval == RESULT::ABORT_FAILURE) {
        data -> nb_aborted++;
      }
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
#elif READ_TXS
void execute_remote_txs(Thread_Data *thread) {
  int unext, last = -1; 
  val_t val = 0, val_2 = 0;
  thread_benchmark* data = thread -> benchmark;

  barrier_cross(&local_barrier);
  
  // Is the first op a write?
  unext = (rand_range_re(&data -> seed, 100) - 1 < data -> update);
    
  while (stop == 0) {
      val = rand_range_re(&data -> seed, data -> range);
      RESULT retval = remote -> get(val);
      if (retval == RESULT::TRUE) {
        data -> nb_found++;
      }
      else if (retval == RESULT::ABORT_FAILURE) {
        data -> nb_aborted++;
      }
      data -> nb_contains++;
  }
  return;
}
#endif

ExperimentResult run(
  const int update,
  const int alternate,
  const int effective,
  const int seed,
  const int duration,
  const long range,
  const int clientsPerServer
) {
  std::vector<std::thread> threads;
  std::vector<Thread_Data> thread_local_data;
  
  //1 Initialize random seed
  initializeRandomizer(seed);

  //2 Initialize Global Stop Condition
  stop = 0;

  //3 Initialize Local Barrier
  barrier_init(&local_barrier, clientsPerServer + 1);

  //4 Spawn Threads and MAP
  for (int i = 0; i < clientsPerServer; i++) {
    thread_local_data.push_back(
      Thread_Data(i, new thread_benchmark(range, update, alternate, effective))
    );
  }

  for (int i = 0; i < clientsPerServer; i++) {
    threads.push_back(
      std::thread(execute_remote_txs, &thread_local_data[i])
    );
  }

  //5 As a single node, await distributed barrier
  //remote -> awaitMaster();

  //6 Record Time and Cross Barrier
  struct timeval start, end;
  struct timespec timeout;
  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;
  barrier_cross(&local_barrier);

  printf("STARTING...\n");
  gettimeofday(&start, NULL);
  nanosleep(&timeout, NULL);

  //7 End Test
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
  int nb_threads = 8;
  long range = DEFAULT_RANGE;
  int seed = DEFAULT_SEED;
  int update = DEFAULT_UPDATE;
  int unit_tx = DEFAULT_LOCKTYPE;
  int alternate = DEFAULT_ALTERNATE;
  int effective = DEFAULT_EFFECTIVE;
  unsigned long reads, effreads, updates, effupds, size = 0, aborted;

	/* parse the command-line options. */
	while((opt = getopt(argc, argv, "Af:d:r:u:")) != -1) {
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
      case 'A':
          alternate = 1;
          break;
      case 'f':
        effective = atoi(optarg);
        break;
		}
	}

  GlobalView environment = Parser::getEnvironment();
  remote = new Remote(environment);

  ExperimentResult result = run(
    update,
    alternate,
    effective,
    seed,
    duration,
    range,
    environment.clientsPerServer
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


  //cleanup
  //remote -> sendData(reads, effreads, updates, effupds, aborted);
  delete remote; //end connections
	exit(0);
}