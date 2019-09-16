// CSE 375/475 Assignment #1
// Fall 2019
//
// Description: This file specifies a custom map implemented using two vectors.
// << use templates for creating a map of generic types >>
// One vector is used for storing keys. One vector is used for storing values.
// The expectation is that items in equal positions in the two vectors correlate.
// That is, for all values of i, keys[i] is the key for the item in values[i].
// Keys in the map are not necessarily ordered.
//
// The specification for each function appears as comments.
// Students are responsible for implementing the simple map as specified.
#pragma once
#include <cassert>
#include <iostream>

#if OPTIMIZED == 0
#include "Node.h"

template <class K, class V>
class simplemap_t {
private:
    Node<K, V>** values;
    const unsigned int buckets;

  public:
    simplemap_t(unsigned int buckets) : buckets(buckets) {
        this -> values = new Node<K, V>*[buckets];
        for (int i = 0; i < buckets; i++) {
            this -> values[i] = new Node<K, V>(0, 0); //sentintel node
        }
    }

    ~simplemap_t() {
        for (int i = 0; i < this -> buckets; i++) {
            Node<K, V>* runner = this -> values[i];
            while (runner != NULL) {
                Node<K, V>* temp = runner;
                runner = runner -> next;
                delete temp;
            }
        }
        delete [] values;
    }

    unsigned int hash(K key) {
        return key % this -> buckets;
    }

    // Insert (key, val) if and only if the key is not currently present in
    // the map.  Returns true on success, false if the key was
    // already present.
    bool insert(K key, V val) {
        Node<K, V>* sentinel = this -> values[this -> hash(key)];
        Node<K, V>* prv = sentinel;
        Node<K, V>* curr = sentinel -> next;
        while (curr != NULL && curr -> key < key) {
            prv = curr;
            curr = curr -> next;
        }

        if (curr == NULL || curr -> key != key) {
            Node<K, V>* insertion = new Node<K, V>(key, val);
            insertion -> next = curr;
            prv -> next = insertion;
            return true;
        }
    	return false;
    }

    // If key is present in the data structure, replace its value with val
    // and return true; if key is not present in the data structure, return
    // false.
    bool update(K key, V val) {
        Node<K, V>* sentinel = this -> values[this -> hash(key)];

        Node<K, V>* curr = sentinel -> next;
        while (curr != NULL && curr -> key < key) {
            curr = curr -> next;
        }

        if (curr != NULL && curr -> key == key) {
            curr -> val = val;
            return true;
        }
        return false;
    }

    // Remove the (key, val) pair if it is present in the data structure.
    // Returns true on success, false if the key was not already present.
    bool remove(K key) {
        Node<K, V>* sentinel = this -> values[this -> hash(key)];

        Node<K, V>* prv = sentinel;
        Node<K, V>* curr = sentinel -> next;
        while (curr != NULL && curr -> key < key) {
            prv = curr;
            curr = curr -> next;
        }

        if (curr != NULL && curr -> key == key) {
            prv -> next = curr -> next;
            delete curr;
            return true;
        }
        return false;
    }

    // If key is present in the map, return a pair consisting of
    // the corresponding value and true. Otherwise, return a pair with the
    // boolean entry set to false.
    // Be careful not to share the memory of the map with application threads, you might
    // get unexpected race conditions
    std::pair<V, bool> lookup(K key) {
        Node<K, V>* sentinel = this -> values[this -> hash(key)];

        Node<K, V>* runner = sentinel -> next;
        while (runner != NULL && runner -> key < key) {
            runner = runner -> next;
        }

        if (runner != NULL && runner -> key == key) {
            return std::make_pair(runner -> val, true);
        }
        return std::make_pair(0, false);
    }

    // Apply a function to each key in the map
    void apply(void (*f)(K, V)) {
        for (int i = 0; i < this -> buckets; i++) {
            Node<K, V>* runner = this -> values[i] -> next;
            while (runner != NULL) {
                f(runner -> key, runner -> val);
                runner = runner -> next;
            }
        }
    }
};

#else 
#include <vector>

/* 
  Specific to the application at hand,
  assumes 1:1 mapping from Key --> Idx --> Value, 
  as well as receiving items in sorted order
*/
template <class K, class V>
class simplemap_t {
private:
    std::vector<K> keys;
    std::vector<V> values;
    const unsigned int buckets;

  public:
    simplemap_t(unsigned int buckets) : buckets(buckets) {}

    ~simplemap_t() {}

    unsigned int hash(K key) {
        return key % this -> buckets;
    }

    bool insert(K key, V val) {
        keys.push_back(key);
        values.push_back(val);
    }

    bool update(K key, V val) {
        values[key] = val;
    }

    bool removeAll() {
        keys.clear();
        values.clear();
    }

    std::pair<V, bool> lookup(K key) {
        return std::make_pair(values[key], true);
    }

    // Apply a function to each key in the map
    void apply(void (*f)(K, V)) {
        for (int i = 0; i < keys.size(); i++) {
            f(keys[i], values[i]);
        }
    }
};

#endif
