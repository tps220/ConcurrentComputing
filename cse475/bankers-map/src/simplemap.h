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

#include <cassert>
#include "Node.h"

template <class K, class V>
class simplemap_t {
private:
    Node<K, V>** values;
    const unsigned int buckets;
    
    Node<K, V>* acquireElement(K key) {
        const unsigned int idx = this -> hash(key);
        Node<K, V>* sentinel = this -> values[idx];
        LOCK(sentinel -> lock);

        if (sentinel -> next == NULL) {
            sentinel -> next = new Node<K, V>(key, val);
            UNLOCK(sentinel -> lock);
            return true;
        }

        Node<K, V>* prv = sentinel;
        Node<K, V>* curr = sentinel -> next;
        while (curr != NULL && curr -> key < key) {
            prv = curr;
            curr = curr -> next;
        }

        bool result;
        if (result = (curr == NULL || curr -> key != key)) {
            Node<K, V>* insertion = new Node<K, V>(key, val);
            insertion -> next = curr;
            prv -> next = insertion;
        }
        UNLOCK(sentinel -> lock);
        return result;

    }

  public:
    simplemap_t(int buckets) : buckets(buckets) {
        this -> values = new Node<K, V>*[buckets];
        for (int i = 0; i < buckets; i++) {
            this -> values[i] = new Node<K, V>(0, 0); //sentintel node
        }
    }

    // Insert (key, val) if and only if the key is not currently present in
    // the map.  Returns true on success, false if the key was
    // already present.
    bool insert(K key, V val) {
        const unsigned int idx = this -> hash(key);
        Node<K, V>* sentinel = this -> values[idx];
        LOCK(sentinel -> lock);

        if (sentinel -> next == NULL) {
            sentinel -> next = new Node<K, V>(key, val);
            UNLOCK(sentinel -> lock);
            return true;
        }

        Node<K, V>* prv = sentinel;
        Node<K, V>* curr = sentinel -> next;
        while (curr != NULL && curr -> key < key) {
            prv = curr;
            curr = curr -> next;
        }

        bool result;
        if (result = (curr == NULL || curr -> key != key)) {
            Node<K, V>* insertion = new Node<K, V>(key, val);
            insertion -> next = curr;
            prv -> next = insertion;
        }
        UNLOCK(sentinel -> lock);
    	return result;
    }

    // If key is present in the data structure, replace its value with val
    // and return true; if key is not present in the data structure, return
    // false.
    bool update(K key, V val) {
        const unsigned int idx = this -> hash(key);
        Node<K, V>* sentinel = this -> values[idx];
        LOCK(sentinel -> lock);

        if (sentinel -> next == NULL) {
            return false;
        }

        Node<K, V>* curr = sentinel -> next;
        while (curr != NULL && curr -> key < key) {
            curr = curr -> next;
        }

        bool result;
        if (result = (curr != NULL && curr -> key == key)) {
            curr -> val = val;
        }
        UNLOCK(sentinel -> lock);
        return result;
    }

    // Remove the (key, val) pair if it is present in the data structure.
    // Returns true on success, false if the key was not already present.
    bool remove(K key) {
        const unsigned int idx = this -> hash(key);
        Node<K, V>* sentinel = this -> values[idx];
        LOCK(sentinel -> lock);

        if (sentinel -> next == NULL) {
            return false;
        }

        Node<K, V>* prv = sentinel;
        Node<K, V>* curr = sentinel -> next;
        while (curr != NULL && curr -> key < key) {
            prv = curr;
            curr = curr -> next;
        }

        bool result;
        if (result = (curr != NULL && curr -> key == key)) {
            prv -> next = curr -> next;
            delete curr;
        }
        UNLOCK(sentinel -> lock);
        return result;
    }

    // If key is present in the map, return a pair consisting of
    // the corresponding value and true. Otherwise, return a pair with the
    // boolean entry set to false.
    // Be careful not to share the memory of the map with application threads, you might
    // get unexpected race conditions
    std::pair<V, bool> lookup(K key) {
        const unsigned int idx = this -> hash(key);
        Node<K, V>* sentinel = this -> values[idx];
        LOCK(sentinel -> lock);

        if (sentinel -> next == NULL) {
            return false;
        }

        Node<K, V>* curr = sentinel -> next;
        while (curr != NULL && curr -> key < key) {
            curr = curr -> next;
        }

        bool result;
        if (result = (curr != NULL && curr -> key == key)) {
            return std::make_pair(curr -> val, true);
        }
        return std::make_pair(0, false);
    }

    bool deposit(K keySource, K keyTarget, float amount) {
        if (keySource == keyTarget) {
            return true;
        }
        const unsigned int sourceIdx = this -> hash(keySource);
        Node<K, V>* sourceSentinel = this -> values[sourceIdx];

        const unsigned int targetIdx = this -> hash(keyTarget);
        Node<K, V>* targetSentinel = this -> values[targetIdx];

        bool retval;
        if (sourceIdx < targetIdx) {
            LOCK(sourceSentinel);
            LOCK(targetSentinel);
            retval = applyDeposit(getElement(sourceSentinel, keySource), getElement(targetSentinel, keyTarget));
            UNLOCK(sourceSentinel);
            UNLOCK(targetSentinel);
        }
        else if (sourceIdx > targetIdx) {
            LOCK(targetSentinel);
            LOCK(sourceSentinel);
            retval = applyDeposit(getElement(sourceSentinel, keySource), getElement(targetSentinel, keyTarget));
            UNLOCK(targetSentinel);
            UNLOCK(sourceSentinel);
        }
        else {
            LOCK(targetSentinel);
            retval = applyDeposit(getElement(sourceSentinel, keySource), getElement(targetSentinel, keyTarget));
            UNLOCK(sourceSentinel);
        }
        return retval;
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
