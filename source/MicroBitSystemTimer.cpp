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
static unsigned long ticks = 0;
static unsigned int tick_period = 0;

// Array of components which are iterated during a system tick
static MicroBitComponent* systemTickComponents[MICROBIT_SYSTEM_COMPONENTS];

// Periodic callback interrupt
static Ticker timer;


/**
  * Initialises the system wide timer.
  * This must be called before any components register to receive periodic periodic callbacks.
  *
  * @param timer_period The initial period between interrupts, in millseconds.
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if period is illegal.
  */
int system_timer_init(int period)
{
    return system_timer_set_period(period);
}

/*
 * Reconfigures the system wide timer to the given period in milliseconds.
 *
 * @param period the new period of the timer in milliseconds
 * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMETER is returned if period < 1
 */
int system_timer_set_period(int period)
{
    if (period < 1)
        return MICROBIT_INVALID_PARAMETER;

    // If a timer is already running, ensure it is disabled before reconfiguring.
    if (tick_period)
        timer.detach();

	// register a period callback to drive the scheduler and any other registered components.
    tick_period = period;
    timer.attach_us(system_timer_tick, period * 1000);

    return MICROBIT_OK;
}

/*
 * Provides the current tick period in milliseconds
 * @return the current tick period in milliseconds
 */
int system_timer_get_period()
{
    return tick_period;
}

/*
 * Determines the time since the device was powered on.
 * @return the current time since poweron in milliseconds
 */
unsigned long system_timer_current_time()
{
    return ticks;
}

/**
  * Timer callback. Called from interrupt context, once per period.
  * Simply checks to determine if any fibers blocked on the sleep queue need to be woken up
  * and made runnable.
  */
void system_timer_tick()
{
    // increment our real-time counter.
    ticks += system_timer_get_period();

    // Update any components registered for a callback
    for(int i = 0; i < MICROBIT_SYSTEM_COMPONENTS; i++)
        if(systemTickComponents[i] != NULL)
            systemTickComponents[i]->systemTick();
}

/**
 * Add a component to the array of system components. This component will then receive
 * period callbacks, once every tick period.
 *
 * @param component The component to add.
 * @return MICROBIT_OK on success. MICROBIT_NO_RESOURCES is returned if further components cannot be supported.
 */
int system_timer_add_component(MicroBitComponent *component)
{
    int i = 0;

    // If we haven't been initialized, bring up the timer with the default period.
    if (tick_period == 0)
        system_timer_init(SYSTEM_TICK_PERIOD_MS);

    while(systemTickComponents[i] != NULL && i < MICROBIT_SYSTEM_COMPONENTS)
        i++;

    if(i == MICROBIT_SYSTEM_COMPONENTS)
        return MICROBIT_NO_RESOURCES;

    systemTickComponents[i] = component;
    return MICROBIT_OK;
}

/**
 * remove a component from the array of system components. this component will no longer receive
 * period callbacks.
 * @param component The component to remove.
 * @return MICROBIT_OK on success. MICROBIT_INVALID_PARAMETER is returned if the given component has not been previous added.
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
