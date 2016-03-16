/**
  * Definitions for the MicroBit system timer.
  *
  * This module provides:
  *
  * 1) a concept of global system time since power up
  * 2) a simple periodic multiplexing API for the underlying mbed implementation.
  *
  * The latter is useful to avoid costs associated with multiple mbed Ticker instances
  * in microbit-dal components, as each incurs a significant additional RAM overhead (circa 80 bytes).
  */
  
#ifndef MICROBIT_SYSTEM_TIMER_H
#define MICROBIT_SYSTEM_TIMER_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"

/**
  * Initialises the system wide timer. 
  * This must be called before any components register to receive periodic periodic callbacks.
  *
  * @param timer_period The initial period between interrupts, in millseconds.
  * @return MICROBIT_OK on success.
  */
int system_timer_init(int period);

/*
 * Reconfigures the system wide timer to the given period in milliseconds.
 *
 * @param period the new period of the timer in milliseconds
 * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMETER is returned if period < 1
 */
int system_timer_set_period(int period);

/*
 * Provides the current tick period in milliseconds
 * @return the current tick period in milliseconds
 */
int system_timer_get_period();

/*
 * Determines the time since the device was powered on.
 * @return the current time since poweron in milliseconds
 */
unsigned long system_timer_current_time();

/**
  * Timer callback. Called from interrupt context, once per period.
  * Simply checks to determine if any fibers blocked on the sleep queue need to be woken up 
  * and made runnable.
  */
void system_timer_tick();

/**
 * add a component to the array of system components. This component will then receive
 * period callbacks, once every tick period.
 *
 * @param component The component to add.
 * @return MICROBIT_OK on success. MICROBIT_NO_RESOURCES is returned if further components cannot be supported.
 */
int system_timer_add_component(MicroBitComponent *component);

/**
 * remove a component from the array of system components. this component will no longer receive
 * period callbacks.
 * @param component The component to remove.
 * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMETER is returned if the given component has not been previous added.
 */
int system_timer_remove_component(MicroBitComponent *component);

/**
 * A simple C/C++ wrapper to allow periodic callbacks to standard C functions transparently.
 */
class MicroBitSystemTimerCallback : MicroBitComponent
{
    void (*fn)(void);

    /**
     * Creates an object that receives periodic callbacks from the system timer,
     * and, in turn, calls a plain C function as provided as a parameter.
     */
    public:
    MicroBitSystemTimerCallback(void (*function)(void))
    {
        fn = function;
        system_timer_add_component(this);
    }

    void systemTick()
    {
        fn();
    }
};

#endif
