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
  * N.B. The need for this should be reviewed in the future, should a different memory allocator be
  * made availiable in the mbed platform.
  *
  * P.S. This is a very simple allocator, therefore not without its weaknesses. Why don't you consider 
  * what these are, and consider the tradeoffs against simplicity...
  *
  * TODO: Consider caching recently freed blocks to improve allocation time.
  */

#ifndef MICROBIT_HEAP_ALLOCTOR_H
#define MICROBIT_HEAP_ALLOCTOR_H

#include "mbed.h"
#include <new> 

// The number of heap segments created.
#define MICROBIT_HEAP_COUNT             2

// Flag to indicate that a given block is FREE/USED
#define MICROBIT_HEAP_BLOCK_FREE		0x80000000

/**
  * Initialise the microbit heap according to the parameters defined in MicroBitConfig.h
  * After this is called, any future calls to malloc, new, free or delete will use the new heap.
  * n.b. only code that #includes MicroBitHeapAllocator.h will use this heap. This includes all micro:bit runtime
  * code, and user code targetting the runtime. External code can choose to include this file, or
  * simply use the standard mbed heap.
  *
  * @return MICROBIT_OK on success, or MICROBIT_NO_RESOURCES if the heap could not be allocted.
  */
int microbit_heap_init();

/**
  * Attempt to allocate a given amount of memory from any of our configured heap areas.
  * @param size The amount of memory, in bytes, to allocate.
  * @return A pointer to the allocated memory, or NULL if insufficient memory is available.
  */
void *microbit_malloc(size_t size);


/**
  * Release a given area of memory from the heap. 
  * @param mem The memory area to release.
  */
void microbit_free(void *mem);

/*
 * Wrapper function to ensure we have an explicit handle on the heap allocator provided 
 * by our underlying platform.
 *
 * @param size The amount of memory to allocate.
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
  * Overrides the 'new' operator globally, and redirects calls to the micro:bit theap allocator.
  */
inline void* operator new(size_t size) throw(std::bad_alloc)
{   
    return microbit_malloc(size);
}

/**
  * Overrides the 'new' operator globally, and redirects calls to the micro:bit theap allocator.
  */
inline void* operator new[](size_t size) throw(std::bad_alloc)
{   
    return microbit_malloc(size);
}

/**
  * Overrides the 'delete' operator globally, and redirects calls to the micro:bit theap allocator.
  */
inline void operator delete(void *ptr) throw()
{ 
    microbit_free(ptr);
}


// Macros to override overrides the 'malloc' and 'delete' functions globally, and redirects calls 
// to the micro:bit theap allocator.  

#define malloc(X) microbit_malloc( X ) 
#define free(X) microbit_free( X ) 

#endif
