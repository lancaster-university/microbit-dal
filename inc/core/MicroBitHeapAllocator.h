/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

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
  * A simple 32 bit block based memory allocator. This allows one or more memory segments to
  * be designated as heap storage, and is designed to run in a static memory area or inside the standard C
  * heap for use by the micro:bit runtime. This is required for several reasons:
  *
  * 1) It reduces memory fragmentation due to the high churn sometime placed on the heap
  * by ManagedTypes, fibers and user code. Underlying heap implentations are often have very simplistic
  * allocation pilicies and suffer from fragmentation in prolonged use - which can cause programs to
  * stop working after a period of time. The algorithm implemented here is simple, but highly tolerant to
  * large amounts of churn.
  *
  * 2) It allows us to reuse the 8K of SRAM set aside for SoftDevice as additional heap storage
  * when BLE is not in use.
  *
  * 3) It gives a simple example of how memory allocation works! :-)
  *
  * P.S. This is a very simple allocator, therefore not without its weaknesses. Why don't you consider
  * what these are, and consider the tradeoffs against simplicity...
  *
  * @note The need for this should be reviewed in the future, if a different memory allocator is
  * made availiable in the mbed platform.
  *
  * TODO: Consider caching recently freed blocks to improve allocation time.
  */

#ifndef MICROBIT_HEAP_ALLOCTOR_H
#define MICROBIT_HEAP_ALLOCTOR_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include <new>

// The maximum number of heap segments that can be created.
#define MICROBIT_MAXIMUM_HEAPS          2

// Flag to indicate that a given block is FREE/USED
#define MICROBIT_HEAP_BLOCK_FREE		0x80000000

/**
  * Create and initialise a given memory region as for heap storage.
  * After this is called, any future calls to malloc, new, free or delete may use the new heap.
  * The heap allocator will attempt to allocate memory from heaps in the order that they are created.
  * i.e. memory will be allocated from first heap created until it is full, then the second heap, and so on.
  *
  * @param start The start address of memory to use as a heap region.
  *
  * @param end The end address of memory to use as a heap region.
  *
  * @return MICROBIT_OK on success, or MICROBIT_NO_RESOURCES if the heap could not be allocated.
  *
  * @note Only code that #includes MicroBitHeapAllocator.h will use this heap. This includes all micro:bit runtime
  * code, and user code targetting the runtime. External code can choose to include this file, or
  * simply use the standard heap.
  */
int microbit_create_heap(uint32_t start, uint32_t end);

/**
  * Create and initialise a heap region within the current the heap region specified
  * by the linker script.
  *
  * If the requested amount is not available, then the amount requested will be reduced
  * automatically to fit the space available.
  *
  * @param ratio The proportion of the underlying heap to allocate.
  *
  * @return MICROBIT_OK on success, or MICROBIT_NO_RESOURCES if the heap could not be allocated.
  */
int microbit_create_nested_heap(float ratio);

/**
  * Attempt to allocate a given amount of memory from any of our configured heap areas.
  *
  * @param size The amount of memory, in bytes, to allocate.
  *
  * @return A pointer to the allocated memory, or NULL if insufficient memory is available.
  */
void *microbit_malloc(size_t size);


/**
  * Release a given area of memory from the heap.
  *
  * @param mem The memory area to release.
  */
void microbit_free(void *mem);

/*
 * Wrapper function to ensure we have an explicit handle on the heap allocator provided
 * by our underlying platform.
 *
 * @param size The amount of memory, in bytes, to allocate.
 *
 * @return A pointer to the memory allocated. NULL if no memory is available.
 */
inline void *native_malloc(size_t size)
{
    return malloc(size);
}

/*
 * Wrapper function to ensure we have an explicit handle on the heap allocator provided
 * by our underlying platform.
 *
 * @param p Pointer to the memory to be freed.
 */
inline void native_free(void *p)
{
    free(p);
}

/**
  * Overrides the 'new' operator globally, and redirects calls to the micro:bit heap allocator.
  */
inline void* operator new(size_t size)
{
    return microbit_malloc(size);
}

/**
  * Overrides the 'new' operator globally, and redirects calls to the micro:bit theap allocator.
  */
inline void* operator new[](size_t size)
{
    return microbit_malloc(size);
}

/**
  * Overrides the 'delete' operator globally, and redirects calls to the micro:bit theap allocator.
  */
inline void operator delete(void *ptr)
{
    microbit_free(ptr);
}


// Macros to override overrides the 'malloc' and 'delete' functions globally, and redirects calls
// to the micro:bit theap allocator.

#define malloc(X) microbit_malloc( X )
#define free(X) microbit_free( X )

#endif
