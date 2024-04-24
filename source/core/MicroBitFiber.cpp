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
  * Functionality definitions for the MicroBit Fiber scheduler.
  *
  * This lightweight, non-preemptive scheduler provides a simple threading mechanism for two main purposes:
  *
  * 1) To provide a clean abstraction for application languages to use when building async behaviour (callbacks).
  * 2) To provide ISR decoupling for EventModel events generated in an ISR context.
  */
#include "MicroBitConfig.h"
#include "MicroBitFiber.h"
#include "MicroBitSystemTimer.h"
#include "ErrorNo.h"
#include "MicroBitDevice.h"

/*
 * Statically allocated values used to create and destroy Fibers.
 * required to be defined here to allow persistence during context switches.
 */
Fiber *currentFiber = NULL;                        // The context in which the current fiber is executing.
static Fiber *forkedFiber = NULL;                  // The context in which a newly created child fiber is executing.
static Fiber *idleFiber = NULL;                    // the idle task - performs a power efficient sleep, and system maintenance tasks.
/*
 * Scheduler state.
 */
static Fiber *runQueue = NULL;                     // The list of runnable fibers.
static Fiber *sleepQueue = NULL;                   // The list of blocked fibers waiting on a fiber_sleep() operation.
static Fiber *waitQueue = NULL;                    // The list of blocked fibers waiting on an event.
static Fiber *fiberPool = NULL;                    // Pool of unused fibers, just waiting for a job to do.
static Fiber *fiberList = NULL;                    // List of all active Fibers (excludes those in the fiberPool)

/*
 * Scheduler wide flags
 */
static uint8_t fiber_flags = 0;


/*
 * Fibers may perform wait/notify semantics on events. If set, these operations will be permitted on this EventModel.
 */
static EventModel *messageBus = NULL;

// Array of components which are iterated during idle thread execution.
static MicroBitComponent* idleThreadComponents[MICROBIT_IDLE_COMPONENTS];

/**
  * Utility function to add the currenty running fiber to the given queue.
  *
  * Perform a simple add at the head, to avoid complexity,
  *
  * Queues are normally very short, so maintaining a doubly linked, sorted list typically outweighs the cost of
  * brute force searching.
  *
  * @param f The fiber to add to the queue
  *
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
        f->qnext = NULL;
        *queue = f;
    }
    else
    {
        // Scan to the end of the queue.
        // We don't maintain a tail pointer to save RAM (queues are nrmally very short).
        Fiber *last = *queue;

        while (last->qnext != NULL)
            last = last->qnext;

        last->qnext = f;
        f->qnext = NULL;
    }

    __enable_irq();
}

/**
  * Utility function to the given fiber from whichever queue it is currently stored on.
  *
  * @param f the fiber to remove.
  */
void dequeue_fiber(Fiber *f)
{
    // If this fiber is already dequeued, nothing the there's nothing to do.
    if (f->queue == NULL)
        return;

    __disable_irq();

    if (*(f->queue) == f)
    {
        // Remove the fiber from the head of the queue
        *(f->queue) = f->qnext;
    }
    else
    {
        Fiber *prev = *(f->queue);

        // Scan for the given fiber in its queue
        while(prev->qnext != f)
            prev = prev->qnext;

        // Remove the fiber
        prev->qnext = f->qnext;
    }

    // Ensure old linkage is cleared
    f->qnext = NULL;
    f->queue = NULL;

    __enable_irq();
}

/**
  * Provides a list of all active fibers.
  * 
  * @return A pointer to the head of the list of all active fibers.
  */
Fiber * get_fiber_list()
{
    return fiberList;
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

#if CONFIG_ENABLED(MICROBIT_FIBER_USER_DATA)
    f->user_data = 0;
#endif

    // Add the new Fiber to the list of all fibers
    __disable_irq();
    f->next = fiberList;
    fiberList = f;
    __enable_irq();

    return f;
}


/**
  * Initialises the Fiber scheduler.
  * Creates a Fiber context around the calling thread, and adds it to the run queue as the current thread.
  *
  * This function must be called once only from the main thread, and before any other Fiber operation.
  *
  * @param _messageBus An event model, used to direct the priorities of the scheduler.
  */
void scheduler_init(EventModel &_messageBus)
{
    // If we're already initialised, then nothing to do.
    if (fiber_scheduler_running())
        return;

	// Store a reference to the messageBus provided.
	// This parameter will be NULL if we're being run without a message bus.
    messageBus = &_messageBus;

    // Create a new fiber context
    currentFiber = getFiberContext();

    // Add ourselves to the run queue.
    queue_fiber(currentFiber, &runQueue);

    // Create the IDLE fiber.
    // Configure the fiber to directly enter the idle task.
    idleFiber = getFiberContext();
    idleFiber->tcb.SP = CORTEX_M0_STACK_BASE - 0x04;
    idleFiber->tcb.LR = (uint32_t) &idle_task;

	if (messageBus)
	{
		// Register to receive events in the NOTIFY channel - this is used to implement wait-notify semantics
		messageBus->listen(MICROBIT_ID_NOTIFY, MICROBIT_EVT_ANY, scheduler_event, MESSAGE_BUS_LISTENER_IMMEDIATE);
		messageBus->listen(MICROBIT_ID_NOTIFY_ONE, MICROBIT_EVT_ANY, scheduler_event, MESSAGE_BUS_LISTENER_IMMEDIATE);
	}

	// register a period callback to drive the scheduler and any other registered components.
    new MicroBitSystemTimerCallback(scheduler_tick);

	fiber_flags |= MICROBIT_SCHEDULER_RUNNING;
}

/**
  * Determines if the fiber scheduler is operational.
  *
  * @return 1 if the fber scheduler is running, 0 otherwise.
  */
int fiber_scheduler_running()
{
	if (fiber_flags & MICROBIT_SCHEDULER_RUNNING)
		return 1;

	return 0;
}

/**
  * The timer callback, called from interrupt context once every SYSTEM_TICK_PERIOD_MS milliseconds.
  * This function checks to determine if any fibers blocked on the sleep queue need to be woken up
  * and made runnable.
  */
void scheduler_tick()
{
    Fiber *f = sleepQueue;
    Fiber *t;

    // Check the sleep queue, and wake up any fibers as necessary.
    while (f != NULL)
    {
        t = f->qnext;

        if (system_timer_current_time() >= f->context)
        {
            // Wakey wakey!
            dequeue_fiber(f);
            queue_fiber(f,&runQueue);
        }

        f = t;
    }
}

/**
  * Event callback. Called from an instance of MicroBitMessageBus whenever an event is raised.
  *
  * This function checks to determine if any fibers blocked on the wait queue need to be woken up
  * and made runnable due to the event.
  *
  * @param evt the event that has just been raised on an instance of MicroBitMessageBus.
  */
void scheduler_event(MicroBitEvent evt)
{
    Fiber *f = waitQueue;
    Fiber *t;
    int notifyOneComplete = 0;

	// This should never happen.
	// It is however, safe to simply ignore any events provided, as if no messageBus if recorded,
	// no fibers are permitted to block on events.
	if (messageBus == NULL)
		return;

    // Check the wait queue, and wake up any fibers as necessary.
    while (f != NULL)
    {
        t = f->qnext;

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
        messageBus->ignore(evt.source, evt.value, scheduler_event);
}

/**
 * Internal utility function to perform a fork operation on the current fiber, and return
 * the current fibers context to the point at which it was checkpointed.
 * 
 * This function is called whenever a fiber requests a "Fork on Block" behaviour and a
 * blocking call to the scheduler is requested.
 */
static Fiber* handle_fob()
{
    Fiber *f = currentFiber;

    // This is a blocking call, so if we're in a fork on block context,
    // it's time to spawn a new fiber...
    if (f->flags & MICROBIT_FIBER_FLAG_FOB)
    {
        // Allocate a TCB from the new fiber. This will come from the tread pool if availiable,
        // else a new one will be allocated on the heap.
        forkedFiber = getFiberContext();
        
        // If we're out of memory, there's nothing we can do.
        // keep running in the context of the current thread as a best effort.
        if (forkedFiber != NULL) {
#if CONFIG_ENABLED(MICROBIT_FIBER_USER_DATA)
            forkedFiber->user_data = f->user_data;
            f->user_data = NULL;
#endif
            f = forkedFiber;
        }
    }
    return f;
}

/**
  * Blocks the calling thread for the given period of time.
  * The calling thread will be immediateley descheduled, and placed onto a
  * wait queue until the requested amount of time has elapsed.
  *
  * @param t The period of time to sleep, in milliseconds.
  *
  * @note the fiber will not be be made runnable until after the elapsed time, but there
  * are no guarantees precisely when the fiber will next be scheduled.
  */
void fiber_sleep(unsigned long t)
{
    // If the scheduler is not running, then simply perform a spin wait and exit.
    if (!fiber_scheduler_running())
    {
        wait_ms(t);
        return;
    }

    // Fork a new fiber if necessary
    Fiber *f = handle_fob();

    // Calculate and store the time we want to wake up.
    f->context = system_timer_current_time() + t;

    // Remove fiber from the run queue
    dequeue_fiber(f);

    // Add fiber to the sleep queue. We maintain strict ordering here to reduce lookup times.
    queue_fiber(f, &sleepQueue);

    // Finally, enter the scheduler.
    schedule();
}

/**
  * Blocks the calling thread until the specified event is raised.
  * The calling thread will be immediateley descheduled, and placed onto a
  * wait queue until the requested event is received.
  *
  * @param id The ID field of the event to listen for (e.g. MICROBIT_ID_BUTTON_A)
  *
  * @param value The value of the event to listen for (e.g. MICROBIT_BUTTON_EVT_CLICK)
  *
  * @return MICROBIT_OK, or MICROBIT_NOT_SUPPORTED if the fiber scheduler is not running, or associated with an EventModel.
  *
  * @code
  * fiber_wait_for_event(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_CLICK);
  * @endcode
  *
  * @note the fiber will not be be made runnable until after the event is raised, but there
  * are no guarantees precisely when the fiber will next be scheduled.
  */
int fiber_wait_for_event(uint16_t id, uint16_t value)
{
    int ret = fiber_wake_on_event(id, value);

    if(ret == MICROBIT_OK)
        schedule();

    return ret;
}

/**
  * Configures the fiber context for the current fiber to block on an event ID
  * and value, but does not deschedule the fiber.
  *
  * @param id The ID field of the event to listen for (e.g. MICROBIT_ID_BUTTON_A)
  *
  * @param value The value of the event to listen for (e.g. MICROBIT_BUTTON_EVT_CLICK)
  *
  * @return MICROBIT_OK, or MICROBIT_NOT_SUPPORTED if the fiber scheduler is not running, or associated with an EventModel.
  *
  * @code
  * fiber_wake_on_event(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_CLICK);
  *
  * //perform some time critical operation.
  *
  * //deschedule the current fiber manually, waiting for the previously configured event.
  * schedule();
  * @endcode
  */
int fiber_wake_on_event(uint16_t id, uint16_t value)
{
	if (messageBus == NULL || !fiber_scheduler_running())
		return MICROBIT_NOT_SUPPORTED;

    // Fork a new fiber if necessary
    Fiber *f = handle_fob();

    // Encode the event data in the context field. It's handy having a 32 bit core. :-)
    f->context = value << 16 | id;

    // Remove ourselves from the run queue
    dequeue_fiber(f);

    // Add ourselves to the sleep queue. We maintain strict ordering here to reduce lookup times.
    queue_fiber(f, &waitQueue);

    // Register to receive this event, so we can wake up the fiber when it happens.
    // Special case for the notify channel, as we always stay registered for that.
    if (id != MICROBIT_ID_NOTIFY && id != MICROBIT_ID_NOTIFY_ONE)
        messageBus->listen(id, value, scheduler_event, MESSAGE_BUS_LISTENER_IMMEDIATE);

    // NOTE: We intentionally don't re-enter the scheduler here, such that this function
    // can be used to create atomic wait events. if using this function, the calling thread MUST
    // call schedule() as its next call to the scheduler.
    return MICROBIT_OK;
}

#if CONFIG_ENABLED(MICROBIT_FIBER_USER_DATA)
#define HAS_THREAD_USER_DATA (currentFiber->user_data != NULL)
#else
#define HAS_THREAD_USER_DATA false
#endif

/**
  * Executes the given function asynchronously if necessary.
  *
  * Fibers are often used to run event handlers, however many of these event handlers are very simple functions
  * that complete very quickly, bringing unecessary RAM overhead.
  *
  * This function takes a snapshot of the current processor context, then attempts to optimistically call the given function directly.
  * We only create an additional fiber if that function performs a block operation.
  *
  * @param entry_fn The function to execute.
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  */
int invoke(void (*entry_fn)(void))
{
    // Validate our parameters.
    if (entry_fn == NULL)
        return MICROBIT_INVALID_PARAMETER;

    if (!fiber_scheduler_running())
		return MICROBIT_NOT_SUPPORTED;

    if (currentFiber->flags & (MICROBIT_FIBER_FLAG_FOB | MICROBIT_FIBER_FLAG_PARENT | MICROBIT_FIBER_FLAG_CHILD) || HAS_THREAD_USER_DATA)
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

#if CONFIG_ENABLED(MICROBIT_FIBER_USER_DATA)
    currentFiber->user_data = 0;
#endif
    currentFiber->flags &= ~MICROBIT_FIBER_FLAG_FOB;

    // If this is is an exiting fiber that for spawned to handle a blocking call, recycle it.
    // The fiber will then re-enter the scheduler, so no need for further cleanup.
    if (currentFiber->flags & MICROBIT_FIBER_FLAG_CHILD)
        release_fiber();

     return MICROBIT_OK;
}

/**
  * Executes the given function asynchronously if necessary, and offers the ability to provide a parameter.
  *
  * Fibers are often used to run event handlers, however many of these event handlers are very simple functions
  * that complete very quickly, bringing unecessary RAM. overhead
  *
  * This function takes a snapshot of the current fiber context, then attempt to optimistically call the given function directly.
  * We only create an additional fiber if that function performs a block operation.
  *
  * @param entry_fn The function to execute.
  *
  * @param param an untyped parameter passed into the entry_fn and completion_fn.
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  */
int invoke(void (*entry_fn)(void *), void *param)
{
    // Validate our parameters.
    if (entry_fn == NULL)
        return MICROBIT_INVALID_PARAMETER;

    if (!fiber_scheduler_running())
		return MICROBIT_NOT_SUPPORTED;

    if (currentFiber->flags & (MICROBIT_FIBER_FLAG_FOB | MICROBIT_FIBER_FLAG_PARENT | MICROBIT_FIBER_FLAG_CHILD) || HAS_THREAD_USER_DATA)
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

#if CONFIG_ENABLED(MICROBIT_FIBER_USER_DATA)
    currentFiber->user_data = 0;
#endif
    currentFiber->flags &= ~MICROBIT_FIBER_FLAG_FOB;

    // If this is is an exiting fiber that for spawned to handle a blocking call, recycle it.
    // The fiber will then re-enter the scheduler, so no need for further cleanup.
    if (currentFiber->flags & MICROBIT_FIBER_FLAG_CHILD)
        release_fiber(param);

    return MICROBIT_OK;
}

/**
 * Launches a fiber.
 *
 * @param ep the entry point for the fiber.
 *
 * @param cp the completion routine after ep has finished execution
 */
void launch_new_fiber(void (*ep)(void), void (*cp)(void))
{
    // Execute the thread's entrypoint
    ep();

    // Execute the thread's completion routine;
    cp();

    // If we get here, then the completion routine didn't recycle the fiber... so do it anyway. :-)
    release_fiber();
}

/**
 * Launches a fiber with a parameter
 *
 * @param ep the entry point for the fiber.
 *
 * @param cp the completion routine after ep has finished execution
 *
 * @param pm the parameter to provide to ep and cp.
 */
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
  *
  * @param completion_fn The function called when the thread completes execution of entry_fn.
  *                      Defaults to release_fiber.
  *
  * @return The new Fiber, or NULL if the operation could not be completed.
  */
Fiber *create_fiber(void (*entry_fn)(void), void (*completion_fn)(void))
{
    if (!fiber_scheduler_running())
		return NULL;

    return __create_fiber((uint32_t) entry_fn, (uint32_t)completion_fn, 0, 0);
}


/**
  * Creates a new parameterised Fiber, and launches it.
  *
  * @param entry_fn The function the new Fiber will begin execution in.
  *
  * @param param an untyped parameter passed into the entry_fn and completion_fn.
  *
  * @param completion_fn The function called when the thread completes execution of entry_fn.
  *                      Defaults to release_fiber.
  *
  * @return The new Fiber, or NULL if the operation could not be completed.
  */
Fiber *create_fiber(void (*entry_fn)(void *), void *param, void (*completion_fn)(void *))
{
    if (!fiber_scheduler_running())
		return NULL;

    return __create_fiber((uint32_t) entry_fn, (uint32_t)completion_fn, (uint32_t) param, 1);
}

/**
  * Exit point for all fibers.
  *
  * Any fiber reaching the end of its entry function will return here  for recycling.
  */
void release_fiber(void *)
{
    if (!fiber_scheduler_running())
		return;

    release_fiber();
}

/**
  * Exit point for all fibers.
  *
  * Any fiber reaching the end of its entry function will return here  for recycling.
  */
void release_fiber(void)
{
    int fiberPoolSize = 0;

    if (!fiber_scheduler_running())
		return;

    // Remove ourselves form the runqueue.
    dequeue_fiber(currentFiber);

    // Scan the FiberPool and release memory to the heap if it is full.
    for (Fiber *p = fiberPool; p; p = p->qnext) 
        fiberPoolSize++;

    while (fiberPoolSize > MICROBIT_FIBER_MAXIMUM_FIBER_POOL_SIZE)
    {
        // Release Fiber contexts from the head of the FiberPool.
        Fiber *p = fiberPool;
        fiberPool = p->qnext;
        free((void *)p->stack_bottom);
        free(p);
        fiberPoolSize--;
    }

    // Add ourselves to the list of free fibers
    queue_fiber(currentFiber, &fiberPool);

    // Remove the fiber from the list of active fibers
    __disable_irq();
    if (fiberList == currentFiber)
    {
        fiberList = fiberList->next;
    }
    else
    {
        Fiber *p = fiberList;

        while (p)
        {
            if (p->next == currentFiber)
            {
                p->next = currentFiber->next;
                break;
            }

            p = p->next;
        }
    }
    __enable_irq();


    // Find something else to do!
    schedule();
}

/**
  * Resizes the stack allocation of the current fiber if necessary to hold the system stack.
  *
  * If the stack allocation is large enough to hold the current system stack, then this function does nothing.
  * Otherwise, the the current allocation of the fiber is freed, and a larger block is allocated.
  *
  * @param f The fiber context to verify.
  *
  * @return The stack depth of the given fiber.
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
        // We are only here when the current stack is the stack of fiber [f].
        // Make sure the contents of [currentFiber] variable reflects that, otherwise
        // an external memory allocator might get confused when scanning fiber stacks.
        Fiber *prevCurrFiber = currentFiber;
        currentFiber = f;

        // To ease heap churn, we choose the next largest multple of 32 bytes.
        bufferSize = (stackDepth + 32) & 0xffffffe0;

        // Release the old memory
        if (f->stack_bottom != 0)
            free((void *)f->stack_bottom);

        // Allocate a new one of the appropriate size.
        f->stack_bottom = (uint32_t) malloc(bufferSize);

        // Recalculate where the top of the stack is and we're done.
        f->stack_top = f->stack_bottom + bufferSize;
        
        // Restore Fiber context
        currentFiber = prevCurrFiber;
    }
}

/**
  * Determines if any fibers are waiting to be scheduled.
  *
  * @return The number of fibers currently on the run queue
  */
int scheduler_runqueue_empty()
{
    return (runQueue == NULL);
}

/**
  * Calls the Fiber scheduler.
  * The calling Fiber will likely be blocked, and control given to another waiting fiber.
  * Call this function to yield control of the processor when you have nothing more to do.
  */
void schedule()
{
    if (!fiber_scheduler_running())
		return;

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
    if (runQueue == NULL)
        currentFiber = idleFiber;

    else if (currentFiber->queue == &runQueue)
        // If the current fiber is on the run queue, round robin.
        currentFiber = currentFiber->qnext == NULL ? runQueue : currentFiber->qnext;

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
        while (runQueue == NULL);

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
  * Adds a component to the array of idle thread components, which are processed
  * when the run queue is empty.
  *
  * @param component The component to add to the array.
  * @return MICROBIT_OK on success or MICROBIT_NO_RESOURCES if the fiber components array is full.
  */
int fiber_add_idle_component(MicroBitComponent *component)
{
    int i = 0;

    while(idleThreadComponents[i] != NULL && i < MICROBIT_IDLE_COMPONENTS)
        i++;

    if(i == MICROBIT_IDLE_COMPONENTS)
        return MICROBIT_NO_RESOURCES;

    idleThreadComponents[i] = component;

    return MICROBIT_OK;
}

/**
  * remove a component from the array of idle thread components
  *
  * @param component the component to remove from the idle component array.
  * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMETER is returned if the given component has not been previously added.
  */
int fiber_remove_idle_component(MicroBitComponent *component)
{
    int i = 0;

    while(idleThreadComponents[i] != component && i < MICROBIT_IDLE_COMPONENTS)
        i++;

    if(i == MICROBIT_IDLE_COMPONENTS)
        return MICROBIT_INVALID_PARAMETER;

    idleThreadComponents[i] = NULL;

    return MICROBIT_OK;
}

/**
  * Set of tasks to perform when idle.
  * Service any background tasks that are required, and attempt a power efficient sleep.
  */
void idle()
{
    // Service background tasks
    for(int i = 0; i < MICROBIT_IDLE_COMPONENTS; i++)
        if(idleThreadComponents[i] != NULL)
            idleThreadComponents[i]->idleTick();

    // If the above did create any useful work, enter power efficient sleep.
    if(scheduler_runqueue_empty())
    	__WFE();
}

/**
  * The idle task, which is called when the runtime has no fibers that require execution.
  *
  * This function typically calls idle().
  */
void idle_task()
{
    while(1)
    {
        idle();
        schedule();
    }
}

/**
 * Create a new lock that can be used for mutual exclusion and condition synchronisation.
 */
MicroBitLock::MicroBitLock()
{
    queue = NULL;
    locked = false;
}

/**
 * Block the calling fiber until the lock is available
 **/
void MicroBitLock::wait()
{
    Fiber *f = currentFiber;

    // If the scheduler is not running, then simply exit, as we're running monothreaded.
    if (!fiber_scheduler_running())
        return;

    if (locked)
    {
        // wait() is a blocking call, so if we're in a fork on block context,
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

        // Remove fiber from the run queue
        dequeue_fiber(f);

        // Add fiber to the sleep queue. We maintain strict ordering here to reduce lookup times.
        queue_fiber(f, &queue);

        // Finally, enter the scheduler.
        schedule();
    }

    locked = true;
}

/**
 * Release the lock, and signal to one waiting fiber to continue
 */
void MicroBitLock::notify()
{
    Fiber *f = queue;

    if (f)
    {
        dequeue_fiber(f);
        queue_fiber(f, &runQueue);
    }
    locked = false;
}

/**
 * Release the lock, and signal to all waiting fibers to continue
 */
void MicroBitLock::notifyAll()
{
    Fiber *f = queue;

    while (f)
    {
        dequeue_fiber(f);
        queue_fiber(f, &runQueue);
        f = queue;
    }

    locked = false;
}
