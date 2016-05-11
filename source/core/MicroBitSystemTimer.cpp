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
#include "MicroBitConfig.h"
#include "MicroBitSystemTimer.h"
#include "ErrorNo.h"

/*
 * Time since power on. Measured in milliseconds.
 * When stored as an unsigned long, this gives us approx 50 days between rollover, which is ample. :-)
 */
static uint64_t time_us = 0;
static unsigned int tick_period = 0;

// Array of components which are iterated during a system tick
static MicroBitComponent* systemTickComponents[MICROBIT_SYSTEM_COMPONENTS];

// Periodic callback interrupt
static Ticker *ticker = NULL;

// System timer.
static Timer *timer = NULL;


/**
  * Initialises a system wide timer, used to drive the various components used in the runtime.
  *
  * This must be called before any components register to receive periodic periodic callbacks.
  *
  * @param timer_period The initial period between interrupts, in millseconds.
  *
  * @return MICROBIT_OK on success.
  */
int system_timer_init(int period)
{
    if (ticker == NULL)
        ticker = new Ticker();

    if (timer == NULL)
    {
        timer = new Timer();
        timer->start();
    }

    return system_timer_set_period(period);
}

/**
  * Reconfigures the system wide timer to the given period in milliseconds.
  *
  * @param period the new period of the timer in milliseconds
  *
  * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMETER is returned if period < 1
  */
int system_timer_set_period(int period)
{
    if (period < 1)
        return MICROBIT_INVALID_PARAMETER;

    // If a timer is already running, ensure it is disabled before reconfiguring.
    if (tick_period)
        ticker->detach();

	// register a period callback to drive the scheduler and any other registered components.
    tick_period = period;
    ticker->attach_us(system_timer_tick, period * 1000);

    return MICROBIT_OK;
}

/**
  * Accessor to obtain the current tick period in milliseconds
  *
  * @return the current tick period in milliseconds
  */
int system_timer_get_period()
{
    return tick_period;
}

/**
  * Updates the current time in microseconds, since power on.
  *
  * If the mbed Timer hasn't been initialised, it will be initialised
  * on the first call to this function.
  */
void update_time()
{
    // If we haven't been initialized, bring up the timer with the default period.
    if (timer == NULL || ticker == NULL)
        system_timer_init(SYSTEM_TICK_PERIOD_MS);

    time_us += timer->read_us();
    timer->reset();
}

/**
  * Determines the time since the device was powered on.
  *
  * @return the current time since power on in milliseconds
  */
uint64_t system_timer_current_time()
{
    return system_timer_current_time_us() / 1000;
}

/**
  * Determines the time since the device was powered on.
  *
  * @return the current time since power on in microseconds
  */
uint64_t system_timer_current_time_us()
{
    update_time();
    return time_us;
}

/**
  * Timer callback. Called from interrupt context, once per period.
  *
  * Simply checks to determine if any fibers blocked on the sleep queue need to be woken up
  * and made runnable.
  */
void system_timer_tick()
{
    update_time();

    // Update any components registered for a callback
    for(int i = 0; i < MICROBIT_SYSTEM_COMPONENTS; i++)
        if(systemTickComponents[i] != NULL)
            systemTickComponents[i]->systemTick();
}

/**
  * Add a component to the array of system components. This component will then receive
  * periodic callbacks, once every tick period.
  *
  * @param component The component to add.
  *
  * @return MICROBIT_OK on success. MICROBIT_NO_RESOURCES is returned if the component array is full.
  *
  * @note The callback will be in interrupt context.
  */
int system_timer_add_component(MicroBitComponent *component)
{
    int i = 0;

    // If we haven't been initialized, bring up the timer with the default period.
    if (timer == NULL || ticker == NULL)
        system_timer_init(SYSTEM_TICK_PERIOD_MS);

    while(systemTickComponents[i] != NULL && i < MICROBIT_SYSTEM_COMPONENTS)
        i++;

    if(i == MICROBIT_SYSTEM_COMPONENTS)
        return MICROBIT_NO_RESOURCES;

    systemTickComponents[i] = component;
    return MICROBIT_OK;
}

/**
  * Remove a component from the array of system components. This component will no longer receive
  * periodic callbacks.
  *
  * @param component The component to remove.
  *
  * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMETER is returned if the given component has not been previously added.
  */
int system_timer_remove_component(MicroBitComponent *component)
{
    int i = 0;

    while(systemTickComponents[i] != component && i < MICROBIT_SYSTEM_COMPONENTS)
        i++;

    if(i == MICROBIT_SYSTEM_COMPONENTS)
        return MICROBIT_INVALID_PARAMETER;

    systemTickComponents[i] = NULL;

    return MICROBIT_OK;
}
