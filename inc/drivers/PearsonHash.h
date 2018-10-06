#ifndef PEARSON_HASH_H
#define PEARSON_HASH_H

#include <stdint.h>
#include "ManagedString.h"

/**
  * A very simple
  */
class PearsonHash
{
    public:

    /**
      * Returns an 8-bit hash of the given ManagedString using the simple pearson hashing algorithm.
      *
      * https://en.wikipedia.org/wiki/Pearson_hashing
      */
    static uint8_t pearsonHash8(ManagedString s);

    /**
      * Returns an 16-bit hash of the given ManagedString using the simple pearson hashing algorithm.
      *
      * Internally, this function just performs two 8-bit hashes modifying the string on the second hash, combining them into a single 16-bit result.
      *
      * https://en.wikipedia.org/wiki/Pearson_hashing
      */
    static uint16_t pearsonHash16(ManagedString s);
};

#endif