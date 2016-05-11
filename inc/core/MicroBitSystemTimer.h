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
  * Initialises a system wide timer, used to drive the various components used in the runtime.
  *
  * This must be called before any components register to receive periodic periodic callbacks.
  *
  * @param timer_period The initial period between interrupts, in millseconds.
  *
  * @return MICROBIT_OK on success.
  */
int system_timer_init(int period);

/**
  * Reconfigures the system wide timer to the given period in milliseconds.
  *
  * @param period the new period of the timer in milliseconds
  *
  * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMETER is returned if period < 1
  */
int system_timer_set_period(int period);

/**
  * Accessor to obtain the current tick period in milliseconds
  *
  * @return the current tick period in milliseconds
  */
int system_timer_get_period();

/**
  * Updates the current time in microseconds, since power on.
  *
  * If the mbed Timer hasn't been initialised, it will be initialised
  * on the first call to this function.
  */
inline void update_time();

/**
  * Determines the time since the device was powered on.
  *
  * @return the current time since power on in milliseconds
  */
uint64_t system_timer_current_time();

/**
  * Determines the time since the device was powered on.
  *
  * @return the current time since power on in microseconds
  */
uint64_t system_timer_current_time_us();

/**
  * Timer callback. Called from interrupt context, once per period.
  *
  * Simply checks to determine if any fibers blocked on the sleep queue need to be woken up
  * and made runnable.
  */
void system_timer_tick();

/**
  * Add a component to the array of system components. This component will then receive
  * periodic callbacks, once every tick period in interrupt context.
  *
  * @param component The component to add.
  *
  * @return MICROBIT_OK on success or MICROBIT_NO_RESOURCES if the component array is full.
  *
  * @code
  * // heap allocated - otherwise it will be paged out!
  * MicroBitDisplay* display = new MicroBitDisplay();
  *
  * system_timer_add_component(display);
  * @endcode
  */
int system_timer_add_component(MicroBitComponent *component);

/**
  * Remove a component from the array of system components. This component will no longer receive
  * periodic callbacks.
  *
  * @param component The component to remove.
  *
  * @return MICROBIT_OK on success or MICROBIT_INVALID_PARAMETER is returned if the given component has not been previously added.
  *
  * @code
  * // heap allocated - otherwise it will be paged out!
  * MicroBitDisplay* display = new MicroBitDisplay();
  *
  * system_timer_add_component(display);
  *
  * system_timer_remove_component(display);
  * @endcode
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
     *
     * @param function the function to invoke upon a systemTick.
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
