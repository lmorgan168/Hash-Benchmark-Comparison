///////////////////////////////////////////////////////////////////////////////
// hashes.hpp
//
// Implementations of four dictionary data structures: naive, chained hash
// table, linear probing hash table, and cuckoo hash table.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace hashes {

  const uint32_t LARGE_PRIME = 2147483647; // largest prime less than 2^31

  class key_exception { };

  // One entry in a dictionary.
  template <typename T>
  class entry {
  public:

    entry(uint32_t key, T&& value) noexcept
    : key_(key), value_(value) { }

    entry() noexcept
    : key_(0), value_(0) { }

    uint32_t key() const noexcept { return key_; }

    const T& value() const noexcept { return value_; }
    T& value() noexcept { return value_; }
    void set_value(T&& value) noexcept { value_ = value; }

  private:
    uint32_t key_;
    T value_;
  };

  // Abstract base class for hash functions.
  class abstract_hash_func {
  public:

    virtual ~abstract_hash_func() { }

    // Evaluate the hash function for the given key.
    virtual uint32_t hash(uint32_t key) const noexcept = 0;
  };

  // Order-2 polynomial, i.e.
  // h(x) = a0 + a1*x
  class poly2_hash_func : public abstract_hash_func {
  public:

    poly2_hash_func() noexcept {
      a_0 = rand() % LARGE_PRIME;       // randomly choose coefficients with LARGE_PRIME
      a_1 = rand() % LARGE_PRIME;
    }

    virtual uint32_t hash(uint32_t key) const noexcept {
      uint32_t index = a_0 + a_1*key;       // create hash function
      return index;
    }

  private:
    int a_0;        // initialize coefficients 
    int a_1;
  };

  // Order-5 polynomial, i.e.
  // h(x) = a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4
  class poly5_hash_func : public abstract_hash_func {
  public:

    poly5_hash_func() noexcept {
      a_0 = rand() % LARGE_PRIME;         // randomly choose coefficients with LARGE_PRIME
      a_1 = rand() % LARGE_PRIME;
      a_2 = rand() % LARGE_PRIME;
      a_3 = rand() % LARGE_PRIME;
      a_4 = rand() % LARGE_PRIME;
    }

    virtual uint32_t hash(uint32_t key) const noexcept {
      unsigned int index = a_0 + a_1*key + a_2*key*key + a_3*key*key*key + a_4*key*key*key*key; // create hash function
      return index;
    }

  private:
    int a_0;          // initialize coefficients
    int a_1;      
    int a_2;
    int a_3;
    int a_4;
  };

  // Tabular-hash function, i.e. (4) 256-element arrays whose elements
  // are XORed together.
  class tabular_hash_func : public abstract_hash_func {
  public:

    tabular_hash_func() noexcept {
      std::generate(T1.begin(), T1.end(), std::rand);     // populate four tables with random numbers 
      std::generate(T2.begin(), T2.end(), std::rand);
      std::generate(T3.begin(), T3.end(), std::rand);
      std::generate(T4.begin(), T4.end(), std::rand);
    }

    virtual uint32_t hash(uint32_t key) const noexcept {
      unsigned char *p = (unsigned char*) &key;           // initialize to grab bits 

      unsigned int index = T1.at(p[0])^T2.at(p[1])^T3.at(p[2])^T4.at(p[3]);     // return bitwise exclusive-or
      return index;
    }

  private:
    std::vector<int> T1 = std::vector<int>(256);      // initialize four tables of size 256
    std::vector<int> T2 = std::vector<int>(256);
    std::vector<int> T3 = std::vector<int>(256);
    std::vector<int> T4 = std::vector<int>(256);
  };

  // Abstract base class for a dictionary (hash table).
  template <typename T>
  class abstract_dict {
  public:

    virtual ~abstract_dict() { }

    // Search for the entry matching key, and return a reference to the
    // corresponding value.
    //
    // Throw std::out_of_range if there is no such key.
    //
    // (It would be better C++ practice to also provide a const overload of
    // this function, but that seems like busy-work for this experimental
    // project, so we're skipping that.)
    virtual T& search(uint32_t key) = 0;

    // Assign key to be associated with val. If key is already in the dictionary,
    // replace that association.
    //
    // Throw std::length_error if the dictionary is too full to add another
    // entry.
    virtual void set(uint32_t key, T&& val) = 0;
  };

  // Naive dictionary (unsorted vector).
  template <typename T>
  class naive_dict : public abstract_dict<T> {
  public:

    // Create an empty dictionary, with the given capacity.
    naive_dict(size_t capacity) {
    }

    virtual T& search(uint32_t key) {
      auto iter = search_iterator(key);
      if (iter != entries_.end()) {
        return iter->value();
      } else {
        throw std::out_of_range("key absent in naive_dict::search");
      }
    }

    virtual void set(uint32_t key, T&& val) {
      auto iter = search_iterator(key);
      if (iter != entries_.end()) {
        iter->set_value(std::move(val));
      } else {
        entries_.emplace_back(key, std::move(val));
      }
    }

  private:

    std::vector<entry<T>> entries_;

    typename std::vector<entry<T>>::iterator search_iterator(uint32_t key) {
      return std::find_if(entries_.begin(),
                          entries_.end(),
                          [&](entry<T>& entry) { return entry.key() == key; });
    }

  };

  // Hash table with chaining.
  template <typename T>
  class chain_dict : public abstract_dict<T> {
  public:

    // Create an empty dictionary, with the given capacity.
    chain_dict(size_t capacity) {
      this->size =  capacity;                           // set hash table capacity 
      entries_.resize(capacity);                        // resize hash table to capacity 
    }

    virtual T& search(uint32_t key) {
      unsigned int bucket = hashfxn.hash(key) % size;    // use polynomial2 hash function on key
      auto iter = search_iterator(key, bucket);           // initialize iterator

      if (iter != entries_.at(bucket).end()) {       // search for corresponding value to key
          return iter->value();                      // return value if found 
      }
      throw std::out_of_range("key absent in chain_dict::search");      // throw exception if not found 
    }

    virtual void set(uint32_t key, T&& val) {
      unsigned int bucket = hashfxn.hash(key) % size;    // use polynomial2 hash function on key
      auto iter = search_iterator(key, bucket);          // initialize iterator to iterate through bucket 

      if (iter != entries_.at(bucket).end()) {      
        iter->set_value(std::move(val));          // update value if found in bucket
      }
      else {
        entries_.at(bucket).emplace_back(key, std::move(val));   //  add to end of bucket if not found
      }
    }

  private:
    int size;       
    std::vector<std::vector<entry<T>>> entries_;       // hash table with buckets as elements 
    poly2_hash_func hashfxn;                           // hash function 

    typename std::vector<entry<T>>::iterator search_iterator(uint32_t key, unsigned int bucket) {    // iterator to search for key
      return std::find_if(entries_.at(bucket).begin(),
                          entries_.at(bucket).end(),
                          [&](entry<T>& entry) { return entry.key() == key; });
    }
  };

  // Hash table with linear probing (LP).
  template <typename T>
  class lp_dict : public abstract_dict<T> {
  public:

    // Create an empty dictionary, with the given capacity.
    lp_dict(size_t capacity) {
      this->size = capacity;                                   // set hash table size to the given capacity
      entries_ = new std::vector<entry<T>*>(size);             // initialize entries_ to point to a vector
      for (int i = 0; i < size; i++) {                           
        entries_->at(i) = nullptr;                             // set all pointers in vector to nullptr
      }
    }

    virtual T& search(uint32_t key) {
      unsigned int index = hashfxn.hash(key) % size;            // use polynomial5 hash function on key
      int counter = 0;                                          // initialize counter to 0 

      while(entries_->at(index) != nullptr){                    // while element at index is not a nullptr
        if (counter++ > size){                                  // if counter goes higher than the capacity (stopping condition)
          throw std::out_of_range("key absent in lp_dict::search");     
        }

        if (entries_->at(index)->key() == key){                 // check if element's key at index is equal to our searched key
          return entries_->at(index)->value();                  // return the value
        }
        index++;                                      // search next index
        index %= size;                                // wrap around end back to beginning of array with mod: i.e. 10%10 = 0.
      }

      throw std::out_of_range("key absent in lp_dict::search");
    }

    virtual void set(uint32_t key, T&& val) {
      entry<T>* temp = new entry<T>(key,std::move(val));           // set temp to entry to insert 

      unsigned int index = hashfxn.hash(key) % size;               // use polynomial5 hash function on key        

      while(entries_->at(index) != nullptr && entries_->at(index)->key() != key){    // check if index is occupied 
        index++;                                                   // increment index 
        index %= size;                                             // wrap around end back to beginning of table 
      }
      // null pointer or key at index is current key
      entries_->at(index) = temp;                                  // set pointer at index to temp 
    }

  private:
    int size;                           // size of hash table
    std::vector<entry<T>*>* entries_;   // hash table is pointer to vector of pointers
    poly5_hash_func hashfxn;            // hash function 
  };
  

  // Cuckoo hash table.
  template <typename T>
  class cuckoo_dict : public abstract_dict<T> {
  public:

    // Create an empty dictionary, with the given capacity.
    cuckoo_dict(size_t capacity) 
    :hashfxn(2){
      this->size = capacity;        // set size of hash table 
      entries_.resize(2);           // create vector for two hash tables 

      for (int i = 0; i < 2; i++){                                  // iterate through vector of hash tables
          entries_.at(i) = new std::vector<entry<T>*>(size);        // create vecter of entry pointers at each location
          for (int j = 0; j < size; j++){                           // iterate through hash table
              entries_.at(i)->at(j) = nullptr;                      // set each pointer to null pointer 
          }
      }
      t = 0;                         // initialize t to first hash table
      lc = 0;                        // initialize loop counter to 0
      c = 5;                         // set constant to 5 
    }

    virtual T& search(uint32_t key) {
      unsigned int index1 = hashfxn.at(0).hash(key) % size;       // generate two indexes using tabular hash function
      unsigned int index2 = hashfxn.at(1).hash(key) % size;

      if (entries_.at(0)->at(index1) != nullptr) {            // index of first table not empty
        if (entries_.at(0)->at(index1)->key() == key){               // check index of first table
          return (entries_.at(0)->at(index1)->value());              // return value if found
        }
        else{
          if (entries_.at(1)->at(index2) != nullptr) {
            if (entries_.at(1)->at(index2)->key() == key) {          // check index of second table 
              return entries_.at(1)->at(index2)->value();          // return value if found
            }
          }
        }
      }
      else {                                                  // index of first table empty 
        if (entries_.at(1)->at(index2) != nullptr) {
            if (entries_.at(1)->at(index2)->key() == key) {        // check index of second table 
              return entries_.at(1)->at(index2)->value();          // return value if found
            }
          }
      }

      throw std::out_of_range("key absent in cuckoo_dict::search");     // throw exception if not found in either index 
    } 

    virtual void set(uint32_t key, T&& val) {
      if (lc == c*log(size)){             // checks for infinite loop and rebuild hash tables
        std::vector<std::vector<entry<T>*>*> temp_entry = entries_;

        for (int i = 0; i < 2; i++){
          for (int j = 0; j < entries_.at(i)->size(); j++){
            entries_.at(i)->at(j) = nullptr;
          }
        }
        hashfxn.at(0) = tabular_hash_func();
        hashfxn.at(1) = tabular_hash_func();
          
        lc = 0;       // set loop counter to 0
        t = 0;        // t = 0 

        for (int i = 0; i < 2; i++) {         // rehashes to new tables for all keys in entries 
          for(int j = 0; j < size; j++) {
            if (temp_entry.at(i)->at(j) != nullptr) {
                entry<T>* temp1 = new entry<T>(temp_entry.at(i)->at(j)->key(), std::move(temp_entry.at(i)->at(j)->value()));
                int index = hashfxn.at(t).hash(temp1->key()) % size;

                if (entries_.at(t)->at(index) == nullptr){
                  entries_.at(t)->at(index) = temp1;
                }

                while(entries_.at(t)->at(index) != nullptr){
                  entry<T>* temp2 = new entry<T>(entries_.at(t)->at(index)->key(), std::move(entries_.at(t)->at(index)->value()));
                  entries_.at(t)->at(index) = temp1;
                  t = 1-t;
                  temp1 = temp2;
                  index = hashfxn.at(t).hash(temp1->key()) % size;
                }
                entries_.at(t)->at(index) = temp1;
              }
          }
        }
      }

      entry<T>* temp1 = new entry<T>(key,std::move(val));         // create new temp entry 
      int index = hashfxn.at(t).hash(key) % size;                 // hash key at t (initially 0)

      if (entries_.at(t)->at(index) == nullptr) {                 // check if index at t is empty 
        entries_.at(t)->at(index) = temp1;                        // insert temp into hash table at index
        return;                                                   // return from function if key inserted 
      }

      // case when index at t is not empty 
      while(entries_.at(t)->at(index) != nullptr) {
        // make temp2 key of key to evict 
        entry<T>* temp2 = new entry<T>(entries_.at(t)->at(index)->key(), std::move(entries_.at(t)->at(index)->value())); 
        entries_.at(t)->at(index) = temp1;      // insert temp1 into index 
        t = 1-t;                                // iterate to other table
        lc++;                                   // increase loop count
        temp1 = temp2;                          // set original temp to evicted key
        index = hashfxn.at(t).hash(temp1->key()) % size;        // rehash evicted key 
      }
      entries_.at(t)->at(index) = temp1;        // place temp key into empty index
    }

  private:
    int size;       // capacity of hash table                           
    int lc;         // loop counter
    int c;          // constant 
    int t;          // number of hash tables
    std::vector<std::vector<entry<T>*>*> entries_;    // vector of vector pointers to entry pointers 
    std::vector<tabular_hash_func> hashfxn;           // vector of hash functions
  };
}
