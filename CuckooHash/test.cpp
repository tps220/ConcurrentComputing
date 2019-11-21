#include <iostream>
#include "Parallelize.hpp"
#include <stdlib.h>
#include <getopt.h>
#include <vector>
#include <assert.h>

#define PRINT_THREAD_LOCAL_DATA

int main(int argc, char **argv) {
  struct option long_options[] = {
    // These options don't set a flag
    { "help",                      no_argument,       NULL, 'h' },
    { "duration",                  required_argument, NULL, 'd' },
    { "initial-size",              required_argument, NULL, 'i' },
    { "thread",                    required_argument, NULL, 't' },
    { "range",                     required_argument, NULL, 'r' },
    { "seed",                      required_argument, NULL, 'S' },
    { "update-rate",               required_argument, NULL, 'u' },
    { "unit-tx",                   required_argument, NULL, 'x' },
    { NULL, 0, NULL, 0 }
  };
  
  int i, c;
  unsigned long reads, effreads, updates, effupds;
  struct timeval start, end;
  struct timespec timeout;
  int duration = DEFAULT_DURATION;
  int initial = DEFAULT_INITIAL;
  int nb_threads = DEFAULT_NB_THREADS;
  long range = DEFAULT_RANGE;
  int seed = DEFAULT_SEED;
  int update = DEFAULT_UPDATE;
  int unit_tx = DEFAULT_LOCKTYPE;
  int alternate = DEFAULT_ALTERNATE;
  int effective = DEFAULT_EFFECTIVE;
  
  while (true) {
    i = 0;
    c = getopt_long(argc, argv, "hAf:d:i:t:r:S:u:x:", long_options, &i);
    
    if (c == -1) {
      break;
    }
    
    if (c == 0 && long_options[i].flag == 0) {
      c = long_options[i].val;
    }
    
    switch (c) {
    case 0:
      /* Flag is automatically set */
      break;
    case 'h':
      printf("intset -- STM stress test "
       "(linked list)\n"
       "\n"
       "Usage:\n"
       "  intset [options...]\n"
       "\n"
       "Options:\n"
       "  -h, --help\n"
       "        Print this message\n"
       "  -A, --alternate (default="XSTR(DEFAULT_ALTERNATE)")\n"
       "        Consecutive insert/remove target the same value\n"
       "  -f, --effective <int>\n"
       "        update txs must effectively write (0=trial, 1=effective, default=" XSTR(DEFAULT_EFFECTIVE) ")\n"
       "  -d, --duration <int>\n"
       "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
       "  -i, --initial-size <int>\n"
       "        Number of elements to insert before test (default=" XSTR(DEFAULT_INITIAL) ")\n"
       "  -t, --thread-num <int>\n"
       "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
       "  -r, --range <int>\n"
       "        Range of integer values inserted in set (default=" XSTR(DEFAULT_RANGE) ")\n"
       "  -S, --seed <int>\n"
       "        RNG seed (0=time-based, default=" XSTR(DEFAULT_SEED) ")\n"
       "  -u, --update-rate <int>\n"
       "        Percentage of update transactions (default=" XSTR(DEFAULT_UPDATE) ")\n"
       );
      exit(0);
    case 'A':
      alternate = 1;
      break;
    case 'f':
      effective = atoi(optarg);
      break;      
    case 'd':
      duration = atoi(optarg);
      break;
    case 'i':
      initial = atoi(optarg);
      break;
    case 't':
      nb_threads = atoi(optarg);
      break;
    case 'r':
      range = atol(optarg);
      break;
    case 'S':
      seed = atoi(optarg);
      break;
    case 'u':
      update = atoi(optarg);
      break;
    case '?':
      printf("Use -h or --help for help\n");
      exit(0);
    default:
      exit(1);
    }
  }
  
  assert(duration >= 0);
  assert(initial >= 0);
  assert(nb_threads > 0);
  assert(range > 0 && range >= initial);
  assert(update >= 0 && update <= 100);
  
  printf("Set type     : abstract\n");
  printf("Length       : %d\n", duration);
  printf("Initial size : %d\n", initial);
  printf("Nb threads   : %d\n", nb_threads);
  printf("Value range  : %ld\n", range);
  printf("Seed         : %d\n", seed);
  printf("Update rate  : %d\n", update);
  printf("Lock alg     : %d\n", unit_tx);
  printf("Alternate    : %d\n", alternate);
  printf("Effective    : %d\n", effective);
  printf("Type sizes   : int=%d/long=%d/ptr=%d/word=%d\n",
    (int)sizeof(int),
    (int)sizeof(long),
    (int)sizeof(void *),
    (int)sizeof(uintptr_t)
  );
  
  /* Populate set */
  Cuckoo<int> *set = new Cuckoo<int>(initial / WIDTH * 1.05);
  val_t last = 0; 
  val_t val = 0;
  printf("Adding %d entries to set\n", initial);
  i = 0;
  while (i < initial) {
    val = (rand() % range) + 1;
    if (set -> insert(val)) {
      last = val;
      i++;
    }
  }
  int size = set -> size();
  printf("Set size     : %d\n", size);

  Parallelize parallelize(set, range, nb_threads);
  ExperimentResult result = parallelize.run(
    last,
    update,
    unit_tx,
    alternate,
    effective,
    seed,
    duration
  );
  
  std::vector<Thread_Data> snapshot = result.data;
  duration = result.duration;

  std::vector<thread_benchmark> data;
  for (i = 0; i < snapshot.size(); i++) {
    data.push_back(*snapshot[i].benchmark);
  }

  reads = 0;
  effreads = 0;
  updates = 0;
  effupds = 0;
  for (i = 0; i < nb_threads; i++) {
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
    
    //size += data[i].diff;
    size += data[i].nb_added - data[i].nb_removed;
  }

  printf("Set size      : %d (expected: %d)\n", set -> size(), size);
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
  
  //Delete set
  
  return 0;
}
