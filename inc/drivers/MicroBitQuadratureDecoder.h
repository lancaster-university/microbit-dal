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

#ifndef MICROBIT_QUADRATUREDECODER_H
#define MICROBIT_QUADRATUREDECODER_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "MicroBitPin.h"

                                                        // Configuration flags...
#define QDEC_USE_SYSTEM_TICK                0x01        // Use systemTick() to keep position up to date.
#define QDEC_USE_DEBOUNCE                   0x02        // Use input debounce feature.
#define QDEC_LED_ACTIVE_LOW                 0x04        // Drive LED pin low to activate.

/**
  * Class definition for MicroBit Quadrature decoder.
  *
  */
class MicroBitQuadratureDecoder : public MicroBitComponent
{
    protected:
    int64_t         position = 0;       // Absolute position
    MicroBitPin&    phaseA,             // Phase A input for decoding
                    phaseB;             // Phase B input for decoding
    MicroBitPin*    LED;                // LED output to assert while decoding
    uint32_t        samplePeriod = 128; // Minimum sampling period allowed
    uint16_t        faults = 0;         // Double-transition counter
    uint8_t         LEDDelay = 0;       // power-up time for LED, in microseconds
    uint8_t         flags;

    public:

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
    MicroBitQuadratureDecoder(MicroBitPin& phaseA, MicroBitPin& phaseB, MicroBitPin& LED, uint8_t LEDDelay = 0, uint8_t flags = 0);
    MicroBitQuadratureDecoder(MicroBitPin& phaseA, MicroBitPin& phaseB, uint8_t flags = 0);

    /**
      * Automatically call poll() from the systemTick() event.
      *
      * This has the effect of keeping the position up to date to within
      * SYSTEM_TICK_PERIOD_MS milliseconds.  The system tick hook is registered
      * during a call to start(), or if start() has already been called then
      * it's registered during this call and automatic polling will begin
      * immediately.
      *
      * This should not be used if poll() is being called in response to
      * another regular event.
      */
    void enableSystemTick();

    /**
      * Do not automatically call poll() from the systemTick() event (this is the default).
      *
      * If start() has already been called then the driver's system tick hook
      * will be unregistered during this call and automatic polling will stop
      * immediately.  In either case the setting is recorded for the next time
      * the driver is started.
      */
    void disableSystemTick();

    /**
      * Set the rate at which input pins are sampled.
      *
      * @param  The maximum interval between samples in microseconds.
      *
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the configuration is invalid.
      */
    int setSamplePeriodUs(uint32_t period);

    /**
      * Returns the current sampling period.
      *
      * @return The sampling period in microseconds.
      */
    uint32_t getSamplePeriod();

    /**
      * Configure the hardware to keep this instance up to date.
      *
      * Several instances can exist so long as no more than one of them is
      * attached to the hardware.  This can be a practical way to control
      * several motors with their own encoders if they run only at different
      * times.
      *
      * While the hardware is active, `poll()` must be called
      *
      * @return MICROBIT_OK on success, MICROBIT_BUSY if the hardware is already attached to another instance, or MICROBIT_INVALID_PARAMETER if the configuration is invalid.
      */
    virtual int start();

    /**
      * Stop the hardware and make it available for use by other instances.
      */
    virtual void stop();

    /** Poll hardware for latest decoder movement and reset the hardware counter to zero.
      *
      * This must be called regularly to prevent the hardware from overflowing.
      * About ten times per second, or less if the attached hardware is
      * guaranteed to count more slowly than 10000 encoder counts per second.
      *
      * This call may be made from systemTick(), or a dedicated motor control ticker interrupt.
      */
    virtual void poll();

    /**
      * Read the absolute position of the encoder at last call to `poll()`.
      *
      * @return current decoder position.
      */
    int64_t getPosition() { return position; }

    /**
      * Reset the position to a known value.
      *
      * This can be used to zero the counter on detection of an index or end-stop signal.
      *
      * @param The value that getPosition() should return at this encoder position.
      */
    virtual void resetPosition(int64_t position = 0);

    /**
      * Read the number of polling errors since start().
      *
      * This value shows the number of times a sample has encountered a
      * double-transition condition, where the direction cannot be decoded
      * because the relative order of edge transitions was not witnessed.
      *
      * Such faults imply that the sampling period is too long.
      *
      * @return total number of faults.
      */
    int64_t getCountFaults() { return faults; }

    /**
      * Destructor for MicroBitQuadratureDecoder.
      *
      * Ensures that stop() gets called if necessary.
      */
    virtual ~MicroBitQuadratureDecoder() override;

    virtual void systemTick() override;
};

#endif
