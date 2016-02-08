#include "MicroBit.h"

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
struct HeapDefinition
{
    uint32_t *heap_start;		// Physical address of the start of this heap.
    uint32_t *heap_end;		    // Physical address of the end of this heap.
};

// Create the necessary heap definitions.
// We use two heaps by default: one for SoftDevice reuse, and one to run inside the mbed heap.
HeapDefinition heap[MICROBIT_HEAP_COUNT] = { }; 

// Scans the status of the heap definition table, and returns the number of INITIALISED heaps.
int microbit_active_heaps()
{
    int heapCount = 0;

    for (int i=0; i < MICROBIT_HEAP_COUNT; i++)
    {
        if(heap[i].heap_start != NULL)
            heapCount++;
    }

    return heapCount;
}

#if CONFIG_ENABLED(MICROBIT_DBG) && CONFIG_ENABLED(MICROBIT_HEAP_DBG)

// Internal diagnostics function.
// Diplays a usage summary about a given heap...
void microbit_heap_print(HeapDefinition &heap)
{
	uint32_t	blockSize;
	uint32_t	*block;
    int         totalFreeBlock = 0;
    int         totalUsedBlock = 0;
    int         cols = 0;

    if (heap.heap_start == NULL)
    {
        uBit.serial.printf("--- HEAP NOT INITIALISED ---\n");
        return;
    }

    uBit.serial.printf("heap_start : %p\n", heap.heap_start);
    uBit.serial.printf("heap_end   : %p\n", heap.heap_end);
    uBit.serial.printf("heap_size  : %d\n", (int)heap.heap_end - (int)heap.heap_start);

	// Disable IRQ temporarily to ensure no race conditions!
    __disable_irq();

	block = heap.heap_start;
	while (block < heap.heap_end)
	{
		blockSize = *block & ~MICROBIT_HEAP_BLOCK_FREE;
        uBit.serial.printf("[%C:%d] ", *block & MICROBIT_HEAP_BLOCK_FREE ? 'F' : 'U', blockSize*4);
        if (cols++ == 20)
        {
            uBit.serial.printf("\n");
            cols = 0;
        }

        if (*block & MICROBIT_HEAP_BLOCK_FREE)
            totalFreeBlock += blockSize;
        else
            totalUsedBlock += blockSize;

		block += blockSize;
    }

	// Enable Interrupts
    __enable_irq();

    uBit.serial.printf("\n");

    uBit.serial.printf("mb_total_free : %d\n", totalFreeBlock*4);
    uBit.serial.printf("mb_total_used : %d\n", totalUsedBlock*4);
}


// Internal diagnostics function.
// Diplays a usage summary about all known heaps...
void microbit_heap_print()
{
    for (int i=0; i < MICROBIT_HEAP_COUNT; i++)
    {
        uBit.serial.printf("\nHEAP %d: \n", i);
        microbit_heap_print(heap[i]);
    }
}

#endif

void microbit_initialise_heap(HeapDefinition &heap)
{
    // Simply mark the entire heap as free.
    *heap.heap_start = ((uint32_t) heap.heap_end - (uint32_t) heap.heap_start) / MICROBIT_HEAP_BLOCK_SIZE;
    *heap.heap_start |= MICROBIT_HEAP_BLOCK_FREE;
}

int
microbit_create_sd_heap(HeapDefinition &heap)
{
    heap.heap_start = 0;
    heap.heap_end = 0;

#if CONFIG_ENABLED(MICROBIT_HEAP_REUSE_SD)

#if CONFIG_ENABLED(MICROBIT_BLE_ENABLED)
    // Reclaim RAM from unusused areas on the BLE stack GATT table.
    heap.heap_start = (uint32_t *)MICROBIT_HEAP_BASE_BLE_ENABLED;
    heap.heap_end = (uint32_t *)MICROBIT_HEAP_SD_LIMIT;
#else    
    // Reclaim all the RAM normally reserved for the Nordic SoftDevice.
    heap.heap_start = (uint32_t *)MICROBIT_HEAP_BASE_BLE_DISABLED;
    heap.heap_end = (uint32_t *)MICROBIT_HEAP_SD_LIMIT;
#endif

    if (heap.heap_end > heap.heap_start)
        microbit_initialise_heap(heap);
#endif

    return MICROBIT_OK;
}

int
microbit_create_nested_heap(HeapDefinition &heap)
{
	uint32_t mb_heap_max;
    void *p;
  
    // Ensure we're configured to use this heap at all. If not, we can safely return.
    if (MICROBIT_HEAP_SIZE <= 0)
       return MICROBIT_INVALID_PARAMETER;

	// Snapshot something at the top of the main heap.
	p = native_malloc(sizeof(uint32_t));

	// Compute the size left in our heap, taking care to ensure it lands on a word boundary.
	mb_heap_max = (uint32_t) (((float)(MICROBIT_HEAP_END - (uint32_t)p)) * MICROBIT_HEAP_SIZE);
	mb_heap_max &= 0xFFFFFFFC;

	// Release our reference pointer.
	native_free(p);

	// Allocate memory for our heap.
    // We do this iteratively, as some build configurations seem to have static limits
    // on heap size... This allows us to be keep going anyway!
    while (heap.heap_start == NULL)
    {
        heap.heap_start = (uint32_t *)native_malloc(mb_heap_max);
        if (heap.heap_start == NULL)
        {
            mb_heap_max -= 32;
            if (mb_heap_max <= 0)
                return MICROBIT_NO_RESOURCES;
        }
    }

	heap.heap_end = heap.heap_start + mb_heap_max / MICROBIT_HEAP_BLOCK_SIZE;
    microbit_initialise_heap(heap);

    return MICROBIT_OK;
}

/**
  * Initialise the microbit heap according to the parameters defined in MicroBitConfig.h
  * After this is called, any future calls to malloc, new, free or delete will use the new heap.
  * n.b. only code that #includes MicroBitHeapAllocator.h will use this heap. This includes all micro:bit runtime
  * code, and user code targetting the runtime. External code can choose to include this file, or
  * simply use the standard mbed heap.
  */
int
microbit_heap_init()
{
    int result;

	// Disable IRQ temporarily to ensure no race conditions!
    __disable_irq();

    result = microbit_create_nested_heap(heap[0]);
    if (result != MICROBIT_OK)
    {
        __enable_irq();
        return MICROBIT_NO_RESOURCES;
    }

    result = microbit_create_sd_heap(heap[1]);
    if (result != MICROBIT_OK)
    {
        __enable_irq();
        return MICROBIT_NO_RESOURCES;
    }

	// Enable Interrupts
    __enable_irq();

#if CONFIG_ENABLED(MICROBIT_DBG) && CONFIG_ENABLED(MICROBIT_HEAP_DBG)
    microbit_heap_print();
#endif    

    return MICROBIT_OK;
}

/**
  * Attempt to allocate a given amount of memory from the given heap.
  * @param size The amount of memory, in bytes, to allocate.
  * @param heap The heap the memory is to be allocated from.
  * @return A pointer to the allocated memory, or NULL if insufficient memory is available.
  */
void *microbit_malloc(size_t size, HeapDefinition &heap)
{
	uint32_t	blockSize = 0;
	uint32_t	blocksNeeded = size % MICROBIT_HEAP_BLOCK_SIZE == 0 ? size / MICROBIT_HEAP_BLOCK_SIZE : size / MICROBIT_HEAP_BLOCK_SIZE + 1;
	uint32_t	*block;
	uint32_t	*next;

	if (size <= 0)
		return NULL;

	// Account for the index block;
	blocksNeeded++;
	
	// Disable IRQ temporarily to ensure no race conditions!
    __disable_irq();

	// We implement a first fit algorithm with cache to handle rapid churn...
    // We also defragment free blocks as we search, to optimise this and future searches.
	block = heap.heap_start;
	while (block < heap.heap_end)
	{
		// If the block is used, then keep looking.
		if(!(*block & MICROBIT_HEAP_BLOCK_FREE))
		{
			block += *block;
			continue;
		}

		blockSize = *block & ~MICROBIT_HEAP_BLOCK_FREE;

		// We have a free block. Let's see if the subsequent ones are too. If so, we can merge... 
		next = block + blockSize;

		while (*next & MICROBIT_HEAP_BLOCK_FREE)
		{
			if (next >= heap.heap_end)
				break;

			// We can merge!
			blockSize += (*next & ~MICROBIT_HEAP_BLOCK_FREE);
			*block = blockSize | MICROBIT_HEAP_BLOCK_FREE;
			
			next = block + blockSize;
		}

		// We have a free block. Let's see if it's big enough. 
        // If so, we have a winner.
		if (blockSize >= blocksNeeded)
			break;

		// Otherwise, keep looking...
		block += blockSize;
	}

	// We're full!
	if (block >= heap.heap_end)
    {
        __enable_irq();
        return NULL;
    }

	// If we're at the end of memory or have very near match then mark the whole segment as in use.
	if (blockSize <= blocksNeeded+1 || block+blocksNeeded+1 >= heap.heap_end)
	{
		// Just mark the whole block as used.
		*block &= ~MICROBIT_HEAP_BLOCK_FREE;
	}
	else
	{
		// We need to split the block.
		uint32_t *splitBlock = block + blocksNeeded;
		*splitBlock = blockSize - blocksNeeded;
		*splitBlock |= MICROBIT_HEAP_BLOCK_FREE;

		*block = blocksNeeded;
	}

	// Enable Interrupts
    __enable_irq();

	return block+1;
}

/**
  * Attempt to allocate a given amount of memory from any of our configured heap areas.
  * @param size The amount of memory, in bytes, to allocate.
  * @return A pointer to the allocated memory, or NULL if insufficient memory is available.
  */
void *microbit_malloc(size_t size)
{
    void *p;

    // Assign the memory from the first heap created that has space.
    for (int i=0; i < MICROBIT_HEAP_COUNT; i++)
    {
        if(heap[i].heap_start != NULL)
        {
            p = microbit_malloc(size, heap[i]);
            if (p != NULL)
            {
#if CONFIG_ENABLED(MICROBIT_DBG) && CONFIG_ENABLED(MICROBIT_HEAP_DBG)
                uBit.serial.printf("microbit_malloc: ALLOCATED: %d [%p]\n", size, p);
#endif    
                return p;
            }
        }
    }

    // If we reach here, then either we have no memory available, or our heap spaces
    // haven't been initialised. Either way, we try the native allocator.

    p = native_malloc(size);
    if (p!= NULL)
    {
#if CONFIG_ENABLED(MICROBIT_DBG) && CONFIG_ENABLED(MICROBIT_HEAP_DBG)
        // Keep everything trasparent if we've not been initialised yet
        if (microbit_active_heaps())
            uBit.serial.printf("microbit_malloc: NATIVE ALLOCATED: %d [%p]\n", size, p);
#endif    
        return p;
    }

    // We're totally out of options (and memory!).
#if CONFIG_ENABLED(MICROBIT_DBG) && CONFIG_ENABLED(MICROBIT_HEAP_DBG)
    // Keep everything trasparent if we've not been initialised yet
    if (microbit_active_heaps())
        uBit.serial.printf("microbit_malloc: OUT OF MEMORY\n");
#endif    
    
#if CONFIG_ENABLED(MICROBIT_PANIC_HEAP_FULL)
    panic(MICROBIT_OOM);
#endif        

    return NULL;
}

/**
  * Release a given area of memory from the heap. 
  * @param mem The memory area to release.
  */
void microbit_free(void *mem)
{
	uint32_t	*memory = (uint32_t *)mem;
	uint32_t	*cb = memory-1;

#if CONFIG_ENABLED(MICROBIT_DBG) && CONFIG_ENABLED(MICROBIT_HEAP_DBG)
    if (microbit_active_heaps())
        uBit.serial.printf("microbit_free:   %p\n", mem);
#endif    
    // Sanity check.
	if (memory == NULL)
       return;
   
    // If this memory was created from a heap registered with us, free it.
    for (int i=0; i < MICROBIT_HEAP_COUNT; i++)
    {
        if(memory > heap[i].heap_start && memory < heap[i].heap_end)
        {
            // The memory block given is part of this heap, so we can simply
	        // flag that this memory area is now free, and we're done.
	        *cb |= MICROBIT_HEAP_BLOCK_FREE;
            return;
        }
    }

    // If we reach here, then the memory is not part of any registered heap.
    // Forward it to the native heap allocator, and let nature take its course...
    native_free(mem);
}

