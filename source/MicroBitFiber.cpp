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
Fiber *currentFiber = NULL;                 // The context in which the current fiber is executing.
Fiber *forkedFiber = NULL;                  // The context in which a newly created child fiber is executing.
Fiber *idleFiber = NULL;                    // IDLE task - performs a power efficient sleep, and system maintenance tasks.

/*
 * Scheduler state.
 */
Fiber *runQueue = NULL;                     // The list of runnable fibers.
Fiber *sleepQueue = NULL;                   // The list of blocked fibers waiting on a fiber_sleep() operation.
Fiber *waitQueue = NULL;                    // The list of blocked fibers waiting on an event.
Fiber *fiberPool = NULL;                    // Pool of unused fibers, just waiting for a job to do.

/*
 * Time since power on. Measured in milliseconds.
 * When stored as an unsigned long, this gives us approx 50 days between rollover, which is ample. :-)
 */
unsigned long ticks = 0;

/*
 * Scheduler wide flags
 */
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

    // Record which queue this fiber is on.
    f->queue = queue;

    // Add the fiber to the tail of the queue. Although this involves scanning the
    // list, it results in fairer scheduling.
    if (*queue == NULL)
    {
        f->next = NULL;
        f->prev = NULL;
        *queue = f;
    }
    else
    {
        // Scan to the end of the queue.
        // We don't maintain a tail pointer to save RAM (queues are nrmally very short).
        Fiber *last = *queue;

        while (last->next != NULL)
            last = last->next;

        last->next = f;
        f->prev = last;
        f->next = NULL;
    }

    __enable_irq();
}

/**
  * Utility function to the given fiber from whichever queue it is currently stored on.
  * @param f the fiber to remove.
  */
void dequeue_fiber(Fiber *f)
{
    // If this fiber is already dequeued, nothing the there's nothing to do.
    if (f->queue == NULL)
        return;

    // Remove this fiber fromm whichever queue it is on.
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
  * Allocates a fiber from the fiber pool if availiable. Otherwise, allocates a new one from the heap.
  */
Fiber *getFiberContext()
{
    Fiber *f;

    __disable_irq();

    if (fiberPool != NULL)
    {
        f = fiberPool;
        dequeue_fiber(f);
        // dequeue_fiber() exits with irqs enabled, so no need to do this again!
    }
    else
    {
        __enable_irq();

        f = new Fiber();

        if (f == NULL)
            return NULL;

        f->stack_bottom = 0;
        f->stack_top = 0;
    }

    // Ensure this fiber is in suitable state for reuse.
    f->flags = 0;
    f->tcb.stack_base = CORTEX_M0_STACK_BASE;

    return f;
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
    currentFiber = getFiberContext();

    // Add ourselves to the run queue.
    queue_fiber(currentFiber, &runQueue);

    // Create the IDLE fiber.
    // Configure the fiber to directly enter the idle task.
    idleFiber = getFiberContext();
    idleFiber->tcb.SP = CORTEX_M0_STACK_BASE - 0x04;
    idleFiber->tcb.LR = (uint32_t) &idle_task;

    // Register to receive events in the NOTIFY channel - this is used to implement wait-notify semantics
    uBit.MessageBus.listen(MICROBIT_ID_NOTIFY, MICROBIT_EVT_ANY, scheduler_event, MESSAGE_BUS_LISTENER_IMMEDIATE);
    uBit.MessageBus.listen(MICROBIT_ID_NOTIFY_ONE, MICROBIT_EVT_ANY, scheduler_event, MESSAGE_BUS_LISTENER_IMMEDIATE);

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
    ticks += uBit.getTickPeriod();

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
    int notifyOneComplete = 0;

    // Check the wait queue, and wake up any fibers as necessary.
    while (f != NULL)
    {
        t = f->next;

        // extract the event data this fiber is blocked on.
        uint16_t id = f->context & 0xFFFF;
        uint16_t value = (f->context & 0xFFFF0000) >> 16;

        // Special case for the NOTIFY_ONE channel...
        if ((evt.source == MICROBIT_ID_NOTIFY_ONE && id == MICROBIT_ID_NOTIFY) && (value == MICROBIT_EVT_ANY || value == evt.value))
        {
            if (!notifyOneComplete)
            {
                // Wakey wakey!
                dequeue_fiber(f);
                queue_fiber(f,&runQueue);
                notifyOneComplete = 1;
            }
        }

        // Normal case.
        else if ((id == MICROBIT_ID_ANY || id == evt.source) && (value == MICROBIT_EVT_ANY || value == evt.value))
        {
            // Wakey wakey!
            dequeue_fiber(f);
            queue_fiber(f,&runQueue);
        }

        f = t;
    }

    // Unregister this event, as we've woken up all the fibers with this match.
    if (evt.source != MICROBIT_ID_NOTIFY && evt.source != MICROBIT_ID_NOTIFY_ONE)
        uBit.MessageBus.ignore(evt.source, evt.value, scheduler_event);
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
    Fiber *f = currentFiber;

    // Sleep is a blocking call, so if we're in a fork on block context,
    // it's time to spawn a new fiber...
    if (currentFiber->flags & MICROBIT_FIBER_FLAG_FOB)
    {
        // Allocate a new fiber. This will come from the fiber pool if availiable,
        // else a new one will be allocated on the heap.
        forkedFiber = getFiberContext();

        // If we're out of memory, there's nothing we can do.
        // keep running in the context of the current thread as a best effort.
        if (forkedFiber != NULL)
                f = forkedFiber;
    }

    // Calculate and store the time we want to wake up.
    f->context = ticks + t;

    // Remove fiber from the run queue
    dequeue_fiber(f);

    // Add fiber to the sleep queue. We maintain strict ordering here to reduce lookup times.
    queue_fiber(f, &sleepQueue);

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
    Fiber *f = currentFiber;

    // Sleep is a blocking call, so if we'r ein a fork on block context,
    // it's time to spawn a new fiber...
    if (currentFiber->flags & MICROBIT_FIBER_FLAG_FOB)
    {
        // Allocate a TCB from the new fiber. This will come from the tread pool if availiable,
        // else a new one will be allocated on the heap.
        forkedFiber = getFiberContext();

        // If we're out of memory, there's nothing we can do.
        // keep running in the context of the current thread as a best effort.
        if (forkedFiber != NULL)
                f = forkedFiber;
    }

    // Encode the event data in the context field. It's handy having a 32 bit core. :-)
    f->context = value << 16 | id;

    // Remove ourselve from the run queue
    dequeue_fiber(f);

    // Add ourselves to the sleep queue. We maintain strict ordering here to reduce lookup times.
    queue_fiber(f, &waitQueue);

    // Register to receive this event, so we can wake up the fiber when it happens.
    // Special case for teh notify channel, as we always stay registered for that.
    if (id != MICROBIT_ID_NOTIFY && id != MICROBIT_ID_NOTIFY_ONE)
        uBit.MessageBus.listen(id, value, scheduler_event, MESSAGE_BUS_LISTENER_IMMEDIATE);

    // Finally, enter the scheduler.
    schedule();
}

/**
  * Executes the given function asynchronously.
  *
  * Fibers are often used to run event handlers, however many of these event handlers are very simple functions
  * that complete very quickly, bringing unecessary RAM overhead.
  *
  * This function takes a snapshot of the current processor context, then attempts to optimistically call the given function directly.
  * We only create an additional fiber if that function performs a block operation.
  *
  * @param entry_fn The function to execute.
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  */
int invoke(void (*entry_fn)(void))
{
    // Validate our parameters.
    if (entry_fn == NULL)
        return MICROBIT_INVALID_PARAMETER;

    if (currentFiber->flags & MICROBIT_FIBER_FLAG_FOB)
    {
        // If we attempt a fork on block whilst already in  fork n block context,
        // simply launch a fiber to deal with the request and we're done.
        create_fiber(entry_fn);
        return MICROBIT_OK;
    }

    // Snapshot current context, but also update the Link Register to
    // refer to our calling function.
    save_register_context(&currentFiber->tcb);

    // If we're here, there are two possibilities:
    // 1) We're about to attempt to execute the user code
    // 2) We've already tried to execute the code, it blocked, and we've backtracked.

    // If we're returning from the user function and we forked another fiber then cleanup and exit.
    if (currentFiber->flags & MICROBIT_FIBER_FLAG_PARENT)
    {
        currentFiber->flags &= ~MICROBIT_FIBER_FLAG_FOB;
        currentFiber->flags &= ~MICROBIT_FIBER_FLAG_PARENT;
        return MICROBIT_OK;
    }

    // Otherwise, we're here for the first time. Enter FORK ON BLOCK mode, and
    // execute the function directly. If the code tries to block, we detect this and
    // spawn a thread to deal with it.
    currentFiber->flags |= MICROBIT_FIBER_FLAG_FOB;
    entry_fn();
    currentFiber->flags &= ~MICROBIT_FIBER_FLAG_FOB;

    // If this is is an exiting fiber that for spawned to handle a blocking call, recycle it.
    // The fiber will then re-enter the scheduler, so no need for further cleanup.
    if (currentFiber->flags & MICROBIT_FIBER_FLAG_CHILD)
        release_fiber();

     return MICROBIT_OK;
}

/**
  * Executes the given parameterized function asynchronously.
  *
  * Fibers are often used to run event handlers, however many of these event handlers are very simple functions
  * that complete very quickly, bringing unecessary RAM overhead.
  *
  * This function takes a snapshot of the current processor context, then attempt to optimistically call the given function directly.
  * We only create an additional fiber if that function performs a block operation.
  *
  * @param entry_fn The function to execute.
  * @param param an untyped parameter passed into the entry_fn.
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  */
int invoke(void (*entry_fn)(void *), void *param)
{
    // Validate our parameters.
    if (entry_fn == NULL)
        return MICROBIT_INVALID_PARAMETER;

    if (currentFiber->flags & (MICROBIT_FIBER_FLAG_FOB | MICROBIT_FIBER_FLAG_PARENT | MICROBIT_FIBER_FLAG_CHILD))
    {
        // If we attempt a fork on block whilst already in a fork on block context,
        // simply launch a fiber to deal with the request and we're done.
        create_fiber(entry_fn, param);
        return MICROBIT_OK;
    }

    // Snapshot current context, but also update the Link Register to
    // refer to our calling function.
    save_register_context(&currentFiber->tcb);

    // If we're here, there are two possibilities:
    // 1) We're about to attempt to execute the user code
    // 2) We've already tried to execute the code, it blocked, and we've backtracked.

    // If we're returning from the user function and we forked another fiber then cleanup and exit.
    if (currentFiber->flags & MICROBIT_FIBER_FLAG_PARENT)
    {
        currentFiber->flags &= ~MICROBIT_FIBER_FLAG_FOB;
        currentFiber->flags &= ~MICROBIT_FIBER_FLAG_PARENT;
        return MICROBIT_OK;
    }

    // Otherwise, we're here for the first time. Enter FORK ON BLOCK mode, and
    // execute the function directly. If the code tries to block, we detect this and
    // spawn a thread to deal with it.
    currentFiber->flags |= MICROBIT_FIBER_FLAG_FOB;
    entry_fn(param);
    currentFiber->flags &= ~MICROBIT_FIBER_FLAG_FOB;

    // If this is is an exiting fiber that for spawned to handle a blocking call, recycle it.
    // The fiber will then re-enter the scheduler, so no need for further cleanup.
    if (currentFiber->flags & MICROBIT_FIBER_FLAG_CHILD)
        release_fiber(param);

    return MICROBIT_OK;
}

void launch_new_fiber(void (*ep)(void), void (*cp)(void))
{
    // Execute the thread's entrypoint
    ep();

    // Execute the thread's completion routine;
    cp();

    // If we get here, then the completion routine didn't recycle the fiber... so do it anyway. :-)
    release_fiber();
}

void launch_new_fiber_param(void (*ep)(void *), void (*cp)(void *), void *pm)
{
    // Execute the thread's entrypoint.
    ep(pm);

    // Execute the thread's completion routine.
    cp(pm);

    // If we get here, then the completion routine didn't recycle the fiber... so do it anyway. :-)
    release_fiber(pm);
}

Fiber *__create_fiber(uint32_t ep, uint32_t cp, uint32_t pm, int parameterised)
{
    // Validate our parameters.
    if (ep == 0 || cp == 0)
        return NULL;

    // Allocate a TCB from the new fiber. This will come from the fiber pool if availiable,
    // else a new one will be allocated on the heap.
    Fiber *newFiber = getFiberContext();

    // If we're out of memory, there's nothing we can do.
    if (newFiber == NULL)
        return NULL;

    newFiber->tcb.R0 = (uint32_t) ep;
    newFiber->tcb.R1 = (uint32_t) cp;
    newFiber->tcb.R2 = (uint32_t) pm;

    // Set the stack and assign the link register to refer to the appropriate entry point wrapper.
    newFiber->tcb.SP = CORTEX_M0_STACK_BASE - 0x04;
    newFiber->tcb.LR = parameterised ? (uint32_t) &launch_new_fiber_param : (uint32_t) &launch_new_fiber;

    // Add new fiber to the run queue.
    queue_fiber(newFiber, &runQueue);

    return newFiber;
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
    return __create_fiber((uint32_t) entry_fn, (uint32_t)completion_fn, 0, 0);
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
    return __create_fiber((uint32_t) entry_fn, (uint32_t)completion_fn, (uint32_t) param, 1);
}

/**
  * Default exit point for all parameterised fibers.
  * Any fiber reaching the end of its entry function will return here for recycling.
  */
void release_fiber(void *)
{
    release_fiber();
}

/**
  * Default exit point for all fibers.
  * Any fiber reaching the end of its entry function will return here for recycling.
  */
void release_fiber(void)
{
    // Remove ourselves form the runqueue.
    dequeue_fiber(currentFiber);

    // Add ourselves to the list of free fibers
    queue_fiber(currentFiber, &fiberPool);

    // Find something else to do!
    schedule();
}

/**
  * Resizes the stack allocation of the current fiber if necessary to hold the system stack.
  *
  * If the stack allocaiton is large enough to hold the current system stack, then this function does nothing.
  * Otherwise, the the current allocation of the fiber is freed, and a larger block is allocated.
  *
  * @param f The fiber context to verify.
  */
void verify_stack_size(Fiber *f)
{
    // Ensure the stack buffer is large enough to hold the stack Reallocate if necessary.
    uint32_t stackDepth;
    uint32_t bufferSize;

    // Calculate the stack depth.
    stackDepth = f->tcb.stack_base - ((uint32_t) __get_MSP());

    // Calculate the size of our allocated stack buffer
    bufferSize = f->stack_top - f->stack_bottom;

    // If we're too small, increase our buffer size.
    if (bufferSize < stackDepth)
    {
        // To ease heap churn, we choose the next largest multple of 32 bytes.
        bufferSize = (stackDepth + 32) & 0xffffffe0;

        // Release the old memory
        if (f->stack_bottom != 0)
            free((void *)f->stack_bottom);

        // Allocate a new one of the appropriate size.
        f->stack_bottom = (uint32_t) malloc(bufferSize);

        // Recalculate where the top of the stack is and we're done.
        f->stack_top = f->stack_bottom + bufferSize;
    }
}

/**
  * Determines if any fibers are waiting to be scheduled.
  * @return '1' if there is at least one fiber currently on the run queue, and '0' otherwise.
  */
int scheduler_runqueue_empty()
{
    return (runQueue == NULL);
}

/**
  * Calls the Fiber scheduler.
  * The calling Fiber will likely be blocked, and control given to another waiting fiber.
  * Call this to yield control of the processor when you have nothing more to do.
  */
void schedule()
{
    // First, take a reference to the currently running fiber;
    Fiber *oldFiber = currentFiber;

    // First, see if we're in Fork on Block context. If so, we simply want to store the full context
    // of the currently running thread in a newly created fiber, and restore the context of the
    // currently running fiber, back to the point where it entered FOB.

    if (currentFiber->flags & MICROBIT_FIBER_FLAG_FOB)
    {
        // Record that the fibers have a parent/child relationship
        currentFiber->flags |= MICROBIT_FIBER_FLAG_PARENT;
        forkedFiber->flags |= MICROBIT_FIBER_FLAG_CHILD;

        // Define the stack base of the forked fiber to be align with the entry point of the parent fiber
        forkedFiber->tcb.stack_base = currentFiber->tcb.SP;

        // Ensure the stack allocation of the new fiber is large enough
        verify_stack_size(forkedFiber);

        // Store the full context of this fiber.
        save_context(&forkedFiber->tcb, forkedFiber->stack_top);

        // We may now be either the newly created thread, or the one that created it.
        // if the MICROBIT_FIBER_FLAG_PARENT flag is still set, we're the old thread, so
        // restore the current fiber to its stored context and we're done.
        if (currentFiber->flags & MICROBIT_FIBER_FLAG_PARENT)
            restore_register_context(&currentFiber->tcb);

        // If we're the new thread, we must have been unblocked by the scheduler, so simply return
        // and continue processing.
        return;
    }

    // We're in a normal scheduling context, so perform a round robin algorithm across runnable fibers.
    // OK - if we've nothing to do, then run the IDLE task (power saving sleep)
    if (runQueue == NULL || fiber_flags & MICROBIT_FLAG_DATA_READY)
        currentFiber = idleFiber;

    else if (currentFiber->queue == &runQueue)
        // If the current fiber is on the run queue, round robin.
        currentFiber = currentFiber->next == NULL ? runQueue : currentFiber->next;

    else
        // Otherwise, just pick the head of the run queue.
        currentFiber = runQueue;

    if (currentFiber == idleFiber && oldFiber->flags & MICROBIT_FIBER_FLAG_DO_NOT_PAGE)
    {
        // Run the idle task right here using the old fiber's stack.
        // Keep idling while the runqueue is empty, or there is data to process.

        // Run in the context of the original fiber, to preserve state of flags...
        // as we are running on top of this fiber's stack.
        currentFiber = oldFiber;

        do
        {
            idle();
        }
        while (runQueue == NULL || fiber_flags & MICROBIT_FLAG_DATA_READY);

        // Switch to a non-idle fiber.
        // If this fiber is the same as the old one then there'll be no switching at all.
        currentFiber = runQueue;
    }

    // Swap to the context of the chosen fiber, and we're done.
    // Don't bother with the overhead of switching if there's only one fiber on the runqueue!
    if (currentFiber != oldFiber)
    {
        // Special case for the idle task, as we don't maintain a stack context (just to save memory).
        if (currentFiber == idleFiber)
        {
            idleFiber->tcb.SP = CORTEX_M0_STACK_BASE - 0x04;
            idleFiber->tcb.LR = (uint32_t) &idle_task;
        }

        if (oldFiber == idleFiber)
        {
            // Just swap in the new fiber, and discard changes to stack and register context.
            swap_context(NULL, &currentFiber->tcb, 0, currentFiber->stack_top);
        }
        else
        {
            // Ensure the stack allocation of the fiber being scheduled out is large enough
            verify_stack_size(oldFiber);

            // Schedule in the new fiber.
            swap_context(&oldFiber->tcb, &currentFiber->tcb, oldFiber->stack_top, currentFiber->stack_top);
        }
    }
}


/**
  * Set of tasks to perform when idle.
  * Service any background tasks that are required, and attmept power efficient sleep.
  */
void idle()
{
    // Service background tasks
    uBit.systemTasks();

    // If the above did create any useful work, enter power efficient sleep.
    if(scheduler_runqueue_empty())
    {
        if (uBit.ble)
            uBit.ble->waitForEvent();
        else
            __WFI();
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
        idle();
        schedule();
    }
}
