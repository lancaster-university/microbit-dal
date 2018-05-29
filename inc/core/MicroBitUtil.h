/*
The MIT License (MIT)

Copyright (c) 2017 Lancaster University.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

/**
 * This file contains functions used to maintain compatability and portability.
 * It also contains constants that are used elsewhere in the DAL.
 */

#ifndef MICROBIT_UTIL_H
#define MICROBIT_UTIL_H

#include "MicroBitConfig.h"

#define CREATE_KEY_VALUE_TABLE(NAME, PAIRS) const KeyValueTable NAME { PAIRS, sizeof(PAIRS) / sizeof(KeyValueTableEntry) };

/**
 * Provides a simple key/value pair lookup table with range lookup support.
 * Normally stored in FLASH to reduce RAM usage. Keys should be pre-sorted
 * in ascending order.
 */

struct KeyValueTableEntry
{
    const uint32_t key;
    const uint32_t value;
};

struct KeyValueTable
{
    const KeyValueTableEntry *data;
    const int length;

    KeyValueTableEntry* find(const uint32_t key) const;
    uint32_t get(const uint32_t key) const;
    uint32_t getKey(const uint32_t key) const;
    bool hasKey(const uint32_t key) const;
};


#endif
