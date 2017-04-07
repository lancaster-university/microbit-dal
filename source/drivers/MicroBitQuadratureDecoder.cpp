/*
The MIT License (MIT)

Copyright (c) 2016-2017 Simon Hosie

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

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitCompat.h"
#include "MicroBitEvent.h"
#include "MicroBitSystemTimer.h"
#include "ErrorNo.h"
#include "MicroBitQuadratureDecoder.h"

/**
  * Constructor.
  * Create a software abstraction of the quadrature decoder.
  *
  * @param phaseA             Pin connected to quadrature encoder output A
  * @param phaseB             Pin connected to quadrature encoder output B
  * @param LED                The pin for the LED to enable during each quadrature reading
  * @param LEDDelay           Number of microseconds after LED activation before sampling
  *
  * @code
  * MicroBitQuadratureDecoder qdec(QDEC_ID, QDEC_PHA, QDEC_PHB, QDEC_LED);
  * @endcode
  */
MicroBitQuadratureDecoder::MicroBitQuadratureDecoder(MicroBitPin& phaseA_, MicroBitPin& phaseB_, MicroBitPin& LED_, uint8_t LEDDelay_, uint8_t flags_)
    : phaseA(phaseA_), phaseB(phaseB_), LED(&LED_), LEDDelay(LEDDelay_), flags(flags_)
{
}

MicroBitQuadratureDecoder::MicroBitQuadratureDecoder(MicroBitPin& phaseA_, MicroBitPin& phaseB_, uint8_t flags_)
    : phaseA(phaseA_), phaseB(phaseB_), LED(NULL), flags(flags_)
{
}

/**
  * Automatically call poll() from the systemTick() event.
  *
  * This has the effect of keeping position up to date to within
  * SYSTEM_TICK_PERIOD_MS milliseconds.  The system tick hook is registered
  * during a call to start(), or if start() has already been called then it's
  * registered during this call and automatic polling will begin immediately.
  *
  * This should not be used if poll() is being called in response to
  * another regular event.
  */
void MicroBitQuadratureDecoder::enableSystemTick()
{
    if (!(flags & QDEC_USE_SYSTEM_TICK))
    {
        flags |= QDEC_USE_SYSTEM_TICK;
        if ((status & MICROBIT_COMPONENT_RUNNING) != 0)
            system_timer_add_component(this);
    }
}

/**
  * Do not automatically call poll() from the systemTick() event.
  *
  * If start() has already been called then the driver's system tick hook
  * will be unregistered during this call and automatic polling will stop
  * immediately.  In either case the setting is recorded for the next time
  * the driver is started.
  */
void MicroBitQuadratureDecoder::disableSystemTick()
{
    flags &= ~QDEC_USE_SYSTEM_TICK;
    if ((status & MICROBIT_COMPONENT_RUNNING) != 0)
        system_timer_remove_component(this);
}

/**
  * Set the rate at which input pins are sampled.
  *
  * @param  The maximum interval between samples in microseconds.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the configuration is invalid.
  */
int MicroBitQuadratureDecoder::setSamplePeriodUs(uint32_t period)
{
    if (period < 128)
        return MICROBIT_INVALID_PARAMETER;
    samplePeriod = period;
    return MICROBIT_OK;
}

/**
  * Returns the current sampling period.
  *
  * @return The sampling period in microseconds.
  */
uint32_t MicroBitQuadratureDecoder::getSamplePeriod()
{
    return samplePeriod;
}

/**
  * Configure the hardware to keep this instance up to date.
  *
  * Several instances can exist so long as no more than one of them is
  * attached to the hardware.  This can be a practical way to control
  * several motors with their own encoders if they only run at different
  * times.
  *
  * @return MICROBIT_OK on success, MICROBIT_BUSY if the hardware is already attached to another instance, or MICROBIT_INVALID_PARAMETER if the configuration is invalid.
  */
int MicroBitQuadratureDecoder::start()
{
    int sampleper;

    faults = 0;

    for (sampleper = 7; sampleper >= 0; --sampleper)
    {
        // Find the highest (most power-efficient) sample period available
        // which is not greater than the configuration.  A longer period could
        // miss input transitions.
        if ((128u << sampleper) <= samplePeriod)
            break;
    }

    if ((status & MICROBIT_COMPONENT_RUNNING) != 0)
        return MICROBIT_BUSY;

    NRF_QDEC->SHORTS = 0;           // No shorts
    NRF_QDEC->INTENCLR = ~0;        // No interrupts
    NRF_QDEC->LEDPOL = (flags & QDEC_LED_ACTIVE_LOW) != 0 ? 0 : 1;
    NRF_QDEC->SAMPLEPER = sampleper;
    NRF_QDEC->REPORTPER = 7;        // Slowest possible reporting (not used)
    NRF_QDEC->PSELLED = LED != NULL ? LED->name : NC;
    NRF_QDEC->PSELA = phaseA.name;
    NRF_QDEC->PSELB = phaseB.name;
    NRF_QDEC->DBFEN = (flags & QDEC_USE_DEBOUNCE) != 0 ? 1 : 0;
    NRF_QDEC->LEDPRE = LEDDelay;

    // If these pins were previously triggering events (eg., when emulating
    // quadrature decoder using transition events) then put a stop to that.
    if (LED != NULL)
        LED->eventOn(MICROBIT_PIN_EVENT_NONE);
    phaseA.eventOn(MICROBIT_PIN_EVENT_NONE);
    phaseB.eventOn(MICROBIT_PIN_EVENT_NONE);

    // This is what all the cool kids are doing, so I'll do it too.
    __NOP();
    __NOP();
    __NOP();

    NRF_QDEC->TASKS_READCLRACC = 1; // Clear accumulators
    NRF_QDEC->ENABLE = 1;

    NRF_QDEC->TASKS_START = 1;
    status |= MICROBIT_COMPONENT_RUNNING;

    if ((flags & QDEC_USE_SYSTEM_TICK) != 0)
        system_timer_add_component(this);

    return MICROBIT_OK;
}

/**
  * Stop the hardware and make it available for use by other instances.
  */
void MicroBitQuadratureDecoder::stop()
{
    if ((flags & QDEC_USE_SYSTEM_TICK) != 0)
        system_timer_remove_component(this);

    if ((status & MICROBIT_COMPONENT_RUNNING) != 0)
    {
        NRF_QDEC->TASKS_STOP = 1;
        NRF_QDEC->ENABLE = 0;
        status &= ~MICROBIT_COMPONENT_RUNNING;
    }
}

/** Poll hardware for latest decoder movement and reset the hardware counter to zero.
  *
  * This must be called regularly to prevent the hardware from overflowing.
  * About ten times per second, or less if the attached hardware is
  * guaranteed to count more slowly than 10000 encoder counts per second.
  *
  * This call may be made from systemTick(), or a dedicated motor control ticker interrupt.
  */
void MicroBitQuadratureDecoder::poll()
{
    NRF_QDEC->TASKS_READCLRACC = 1;
    position += (int32_t)NRF_QDEC->ACCREAD;
    faults = min(UINT16_MAX, faults + NRF_QDEC->ACCDBLREAD);
}

/**
  * Reset the position to a known value.
  *
  * This can be used to zero the counter on detection of an index or end-stop signal.
  *
  * @param The value that getPosition() should return at this encoder position.
  */
void MicroBitQuadratureDecoder::resetPosition(int64_t position)
{
    this->position = position;
}

/**
  * Destructor for MicroBitQuadratureDecoder.
  *
  * Ensures that stop() gets called if necessary.
  */
MicroBitQuadratureDecoder::~MicroBitQuadratureDecoder()
{
    stop();
}

void MicroBitQuadratureDecoder::systemTick()
{
    poll();
}

