#pragma once
#include <iostream>
#include <vector>
#include "utils.hpp"
#include "Cuckoo.hpp"

class Thread_Data {
public:
  //Unique Identifier
  int id;

  //Synchrobench specific data
  thread_benchmark *benchmark;

  Thread_Data(int id, thread_benchmark *benchmark);
};

class ExperimentResult {
public:
  int duration;
  std::vector<Thread_Data> data;
  ExperimentResult(int duration, std::vector<Thread_Data> data);
};

class Parallelize {
public:
  const unsigned int range;
  const unsigned int nb_threads;
  Cuckoo<int>* set;

  Parallelize(Cuckoo<int>* set, unsigned int range, unsigned int nb_threads);

  ExperimentResult run(
    const val_t first,
    const int update,
    const int unit_tx,
    const int alternate,
    const int effective,
    const int seed,
    const int duration
  );
};

void execute_local_txs(Thread_Data *thread, Cuckoo<int>* set);
