// CSE 375/475 Assignment #1
// Fall 2019
//
// Description: This file declares a struct for storing per-execution configuration information.
#pragma once
#include <iostream>
#include <string>
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// store all of our command-line configuration parameters

struct config_t {
    // The maximum key value
    int key_max;

    // The number of iterations for which a test should run
    int iters;

    // A string that is output with all the other information
    std::string name;

    // The number of threads to use
    int threads;

    // The number of buckets to use
    unsigned int buckets;

    // simple constructor
    config_t() : key_max(256), iters(1024), name("no_name"), threads(1), buckets(MIN(key_max, 512)) {}

    // Print the values of the iters, and name fields
    void dump();
};
