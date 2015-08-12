/**
  * The MicroBit Fiber scheduler.
  *
  * This lightweight, non-preemptive scheduler provides a simple threading mechanism for two main purposes:
  *
  * 1) To provide a clean abstraction for application languages to use when building async behaviour (callbacks).
  * 2) To provide ISR decoupling for Messagebus events generted in an ISR context.
  */
  
#include "MicroBit.h"

/*
 * Statically allocated values used to create and destroy Fibers.
 * required to be defined here to allow persistence during context switches.
 */
 
/*
 * Scheduler state.
 */
 
Fiber *currentFiber = NULL;                 // The context in which the current fiber is executing.
Fiber *runQueue = NULL;                     // The list of runnable fibers.
Fiber *sleepQueue = NULL;                   // The list of blocked fibers waiting on a fiber_sleep() operation.
Fiber *waitQueue = NULL;                    // The list of blocked fibers waiting on an event.
Fiber *idle = NULL;                         // IDLE task - performs a power efficient sleep, and system maintenance tasks.
Fiber *fiberPool = NULL;                    // Pool of unused fibers, just waiting for a job to do.

Cortex_M0_TCB  *emptyContext = NULL;        // Initialized context for fiber entry state.

/*
 * Time since power on. Measured in milliseconds.
 * When stored as an unsigned long, this gives us approx 50 days between rollover, which is ample. :-)
 */
unsigned long ticks = 0;

uint8_t fiber_flags = 0;

/**
  * Utility function to add the currenty running fiber to the given queue. 
  * Perform a simple add at the head, to avoid complexity,
  * Queues are normally very short, so maintaining a doubly linked, sorted list typically outweighs the cost of
  * brute force searching.
  *
  * @param f The fiber to add to the queue
  * @param queue The run queue to add the fiber to.
  */
void queue_fiber(Fiber *f, Fiber **queue)
{
    __disable_irq();

    f->queue = queue;
    f->next = *queue;
    f->prev = NULL;
    
    if(*queue != NULL)
        (*queue)->prev = f;
        
    *queue = f;    

    __enable_irq();
}

/**
  * Utility function to the given fiber from whichever queue it is currently stored on. 
  * @param f the fiber to remove.
  */
void dequeue_fiber(Fiber *f)
{
    __disable_irq();
    
    if (f->prev != NULL)
        f->prev->next = f->next;
    else
        *(f->queue) = f->next;
    
    if(f->next)
        f->next->prev = f->prev;
        
    f->next = NULL;
    f->prev = NULL;
    f->queue = NULL;
    
    __enable_irq();

}


/**
  * Initialises the Fiber scheduler. 
  * Creates a Fiber context around the calling thread, and adds it to the run queue as the current thread.
  *
  * This function must be called once only from the main thread, and before any other Fiber operation.
  */
void scheduler_init()
{
    // Create a new fiber context
    currentFiber = new Fiber();
    currentFiber->stack_bottom = (uint32_t) malloc(FIBER_STACK_SIZE);
    currentFiber->stack_top = ((uint32_t) currentFiber->stack_bottom) + FIBER_STACK_SIZE;
    
    // Add ourselves to the run queue.
    queue_fiber(currentFiber, &runQueue);
 
    // Build a fiber context around the current thread.
    swap_context(&currentFiber->tcb, &currentFiber->tcb, currentFiber->stack_top, currentFiber->stack_top);    
    
    // Create the IDLE task. This will actually scheduk the IDLE task, but it will immediately yeld back to us.
    // Remove it from the run queue though, as IDLE is a special case.
    idle = create_fiber(idle_task);    
    dequeue_fiber(idle);    
    
    // Flag that we now have a scheduler running
    uBit.flags |= MICROBIT_FLAG_SCHEDULER_RUNNING;
}

/**
  * Timer callback. Called from interrupt context, once every FIBER_TICK_PERIOD_MS milliseconds.
  * Simply checks to determine if any fibers blocked on the sleep queue need to be woken up 
  * and made runnable.
  */
void scheduler_tick()
{
    Fiber *f = sleepQueue;
    Fiber *t;
    
    // increment our real-time counter.
    ticks += FIBER_TICK_PERIOD_MS;
    
    // Check the sleep queue, and wake up any fibers as necessary.
    while (f != NULL)
    {
        t = f->next;        
        
        if (ticks >= f->context)
        {
            // Wakey wakey!
            dequeue_fiber(f);
            queue_fiber(f,&runQueue);
        }
        
        f = t;
    }
}

/**
  * Event callback. Called from the message bus whenever an event is raised. 
  * Checks to determine if any fibers blocked on the wait queue need to be woken up 
  * and made runnable due to the event.
  */
void scheduler_event(MicroBitEvent evt)
{
    Fiber *f = waitQueue;
    Fiber *t;
    
    // Check the wait queue, and wake up any fibers as necessary.
    while (f != NULL)
    {
        t = f->next;        
    
        // extract the event data this fiber is blocked on.    
        uint16_t id = f->context & 0xFF;
        uint16_t value = (f->context & 0xFF00) >> 16;
        
        if ((id == MICROBIT_ID_ANY || id == evt.source) && (value == MICROBIT_EVT_ANY || value == evt.value))
        {
            // Wakey wakey!
            dequeue_fiber(f);
            queue_fiber(f,&runQueue);
        }
        
        f = t;
    }
}


/**
  * Blocks the calling thread for the given period of time.
  * The calling thread will be immediatley descheduled, and placed onto a 
  * wait queue until the requested amount of time has elapsed. 
  * 
  * n.b. the fiber will not be be made runnable until after the elasped time, but there
  * are no guarantees precisely when the fiber will next be scheduled.
  *
  * @param t The period of time to sleep, in milliseconds.
  */
void fiber_sleep(unsigned long t)
{
    // Calculate and store the time we want to wake up.
    currentFiber->context = ticks + t;
    
    // Remove ourselve from the run queue
    dequeue_fiber(currentFiber);
        
    // Add ourselves to the sleep queue. We maintain strict ordering here to reduce lookup times.
    queue_fiber(currentFiber, &sleepQueue);
    
    // Finally, enter the scheduler.
    schedule();
}

/**
  * Blocks the calling thread until the specified event is raised.
  * The calling thread will be immediatley descheduled, and placed onto a 
  * wait queue until the requested event is received.
  * 
  * n.b. the fiber will not be be made runnable until after the event is raised, but there
  * are no guarantees precisely when the fiber will next be scheduled.
  *
  * @param id The ID field of the event to listen for (e.g. MICROBIT_ID_BUTTON_A)
  * @param value The VALUE of the event to listen for (e.g. MICROBIT_BUTTON_EVT_CLICK)
  */
void fiber_wait_for_event(uint16_t id, uint16_t value)
{
    // Encode the event data in the context field. It's handy having a 32 bit core. :-)
    currentFiber->context = value << 16 | id;
    
    // Remove ourselve from the run queue
    dequeue_fiber(currentFiber);
        
    // Add ourselves to the sleep queue. We maintain strict ordering here to reduce lookup times.
    queue_fiber(currentFiber, &waitQueue);
    
    // Finally, enter the scheduler.
    schedule();
}


Fiber *getFiberContext()
{
    Fiber *f;
    
    __disable_irq();
        
    if (fiberPool != NULL)
    {
        f = fiberPool;
        dequeue_fiber(f);
        
        // dequeue_fiber() exits with irqs enablesd, so no need to do this again!
    }
    else
    {
        __enable_irq();
        
        f = new Fiber();
        
        if (f == NULL)
            return NULL;

        f->stack_bottom = (uint32_t) malloc(FIBER_STACK_SIZE);
        f->stack_top = f->stack_bottom + FIBER_STACK_SIZE;
    
        if (f->stack_bottom == NULL)
        {
            delete f;
            return NULL;
        }
    }    
    
    return f;
}

void launch_new_fiber()
{
    // Launch into the entry point, now we're in the correct context. 
    //uint32_t ep = currentFiber->stack_bottom;
    uint32_t ep = *((uint32_t *)(currentFiber->stack_bottom + 0));
    uint32_t cp = *((uint32_t *)(currentFiber->stack_bottom + 4));

    // Execute the thread's entrypoint
    (*(void(*)())(ep))();

    // Execute the thread's completion routine;
    (*(void(*)())(cp))();

    // If we get here, then the completion routine didn't recycle the fiber
    // so do it anyway. :-)
    release_fiber();
}

/**
 * Creates a new Fiber, and launches it.
  *
  * @param entry_fn The function the new Fiber will begin execution in.
  * @param completion_fn The function called when the thread completes execution of entry_fn.  
  * @return The new Fiber.
  */
Fiber *create_fiber(void (*entry_fn)(void), void (*completion_fn)(void))
{
    // Validate our parameters.
    if (entry_fn == NULL || completion_fn == NULL)
        return NULL;
    
    // Allocate a TCB from the new fiber. This will come from the tread pool if availiable,
    // else a new one will be allocated on the heap.
    Fiber *newFiber = getFiberContext();
    
    // If we're out of memory, there's nothing we can do.
    if (newFiber == NULL)
        return NULL;
    
    *((uint32_t *)newFiber->stack_bottom) = (uint32_t) entry_fn;
    *((uint32_t *)(newFiber->stack_bottom+4)) = (uint32_t) completion_fn;
    
    // Use cache fiber state if we have it. This is faster, and safer if we're called from
    // an interrupt context.
    if (emptyContext != NULL)
    {
        memcpy(&newFiber->tcb, emptyContext, sizeof(Cortex_M0_TCB));
    }
    else
    {
        // Otherwise, initialize the TCB from the current context.
        save_context(&newFiber->tcb, newFiber->stack_top);
        
        // Assign the new stack pointer and entry point.
        newFiber->tcb.SP = CORTEX_M0_STACK_BASE;    
        newFiber->tcb.LR = (uint32_t) &launch_new_fiber;
        
        // Store this context for later use.
        emptyContext = (Cortex_M0_TCB *) malloc (sizeof(Cortex_M0_TCB));
        memcpy(emptyContext, &newFiber->tcb, sizeof(Cortex_M0_TCB));
    }
    
    // Add new fiber to the run queue.
    queue_fiber(newFiber, &runQueue);
        
    return newFiber;
}

void launch_new_fiber_param()
{
    // Launch into the entry point, now we're in the correct context. 
    uint32_t ep = *((uint32_t *)(currentFiber->stack_bottom + 0));
    uint32_t pm = *((uint32_t *)(currentFiber->stack_bottom + 4));
    uint32_t cp = *((uint32_t *)(currentFiber->stack_bottom + 8));

    // Execute the thread's entry routine.
    (*(void(*)(void *))(ep))((void *)pm);

    // Execute the thread's completion routine.
    // Execute the thread's entry routine.
    (*(void(*)(void *))(cp))((void *)pm);

    // If we get here, then recycle the fiber context.
    release_fiber((void *)pm);
}

/**
  * Creates a new parameterised Fiber, and launches it.
  *
  * @param entry_fn The function the new Fiber will begin execution in.
  * @param param an untyped parameter passed into the entry_fn anf completion_fn.
  * @param completion_fn The function called when the thread completes execution of entry_fn.  
  * @return The new Fiber.
  */
Fiber *create_fiber(void (*entry_fn)(void *), void *param, void (*completion_fn)(void *))
{
    // Validate our parameters.
    if (entry_fn == NULL || completion_fn == NULL)
        return NULL;
    
    // Allocate a TCB from the new fiber. This will come from the tread pool if availiable,
    // else a new one will be allocated on the heap.
    Fiber *newFiber = getFiberContext();
    
    // If we're out of memory, there's nothing we can do.
    if (newFiber == NULL)
        return NULL;
    
    *((uint32_t *)newFiber->stack_bottom) = (uint32_t) entry_fn;
    *((uint32_t *)(newFiber->stack_bottom+4)) = (uint32_t) param;
    *((uint32_t *)(newFiber->stack_bottom+8)) = (uint32_t) completion_fn;
    
    // Use cache fiber state. This is safe, as the empty context is always created in the non-paramterised
    // version of create_fiber before we're ever called.
    memcpy(&newFiber->tcb, emptyContext, sizeof(Cortex_M0_TCB));

    // Assign the link register to refer to the thread entry point in THIS function.
    newFiber->tcb.LR = (uint32_t) &launch_new_fiber_param;
    
    // Add new fiber to the run queue.
    queue_fiber(newFiber, &runQueue);
        
    return newFiber;
}



/**
  * Exit point for parameterised fibers.
  * A wrapper around release_fiber() to enable transparent operaiton.
  */
void release_fiber(void *param)
{  
    release_fiber();
}


/**
  * Exit point for all fibers.
  * Any fiber reaching the end of its entry function will return here  for recycling.
  */
void release_fiber(void)
{  
    // Remove ourselves form the runqueue.
    dequeue_fiber(currentFiber);
    queue_fiber(currentFiber, &fiberPool);
    
    // Find something else to do!
    schedule();   
}

/**
  * Calls the Fiber scheduler.
  * The calling Fiber will likely be blocked, and control given to another waiting fiber.
  * Call this to yield control of the processor when you have nothing more to do.
  */
void schedule()
{
    // Just round robin for now!
    Fiber *oldFiber = currentFiber;

    // OK - if we've nothing to do, then run the IDLE task (power saving sleep)
    if (runQueue == NULL || fiber_flags & MICROBIT_FLAG_DATA_READ)
        currentFiber = idle;
        
    // If the current fiber is on the run queue, round robin.
    else if (currentFiber->queue == &runQueue)
        currentFiber = currentFiber->next == NULL ? runQueue : currentFiber->next;

    // Otherwise, just pick the head of the run queue.
    else
        currentFiber = runQueue;
        
    // Swap to the context of the chosen fiber, and we're done.
    // Don't bother with the overhead of switching if there's only one fiber on the runqueue!
    if (currentFiber != oldFiber)        
    {
        // Ensure the stack buffer is large enough to hold the stack Reallocate if necessary.
        uint32_t stackDepth;
        uint32_t bufferSize;
        
        // Calculate the stack depth.
        stackDepth = CORTEX_M0_STACK_BASE - ((uint32_t) __get_MSP());
        bufferSize = oldFiber->stack_top - oldFiber->stack_bottom;
        
        // If we're too small, increase our buffer exponentially.
        if (bufferSize < stackDepth)
        {
            while (bufferSize < stackDepth)
                bufferSize = bufferSize << 1;
                    
            free((void *)oldFiber->stack_bottom);
            oldFiber->stack_bottom = (uint32_t) malloc(bufferSize);
            oldFiber->stack_top = oldFiber->stack_bottom + bufferSize;
        }

        // Schedule in the new fiber.
        swap_context(&oldFiber->tcb, &currentFiber->tcb, oldFiber->stack_top, currentFiber->stack_top);
    }
}

/**
  * IDLE task.
  * Only scheduled for execution when the runqueue is empty.
  * Performs a procressor sleep operation, then returns to the scheduler - most likely after a timer interrupt.
  */
void idle_task()
{
    while(1)
    {
        if (uBit.ble)
            uBit.ble->waitForEvent();
        else
            __WFI();
            
        uBit.systemTasks();

        schedule();
    }
}
