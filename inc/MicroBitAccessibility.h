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

#ifndef MICROBIT_ACCESSIBILITY_H
#define MICROBIT_ACCESSIBILITY_H

#include "mbed.h"
#include "MicroBitComponent.h"

/**
  * Status flags
  */
#define MICROBIT_ACCESSIBILITY_ENABLED       0x01

/**
 * This class provides functionality to export the state of the LED matrix display
 * over a given serial port. The aim of this is to enable the use of accessbility tools that
 * can replace the LED matric display with an alternate output device, such as braille.
 */
class MicroBitAccessibility : public MicroBitComponent
{
    public:

    /**
     * Constructor.
     * Create an accessbility object that can intercept updates to the LED display and
     * transmit a representation of this over over the serial port using a simple JSON fomrat.
     *
     * @param id the ID of the new object.
     *
     * Example:
     * @code
     * MicroBitAccessibility a(MICROBIT_ID_ACCESSIBILITY)
     * @endcode
     */
    MicroBitAccessibility(uint16_t id);

    /**
     * Enables accessibility behaviour on this micro:bit.
     * Also updates this configuration in persistent storage for future use.
     *
     * @return MICROBIT_OK on success.
     *
     * Example:
     * @code
     * uBit.accessibility.enable();
     * @endcode
     */
    int enable();

    /**
     * Disables accessibility behaviour on this micro:bit.
     * Also updates this configuration in persistent storage for future use.
     *
     * @return MICROBIT_OK on success.
     *
     * Example:
     * @code
     * uBit.accessibility.disable();
     * @endcode
     */
    int disable();

    /**
     * Periodic callback from MicroBit clock.
     * Check if any data is ready for reading by checking the interrupt flag on the accelerometer
     */
    void idleTick();

    /**
     * Destructor for MicroBitAccessibility, so that we deregister ourselves as an idleComponent
     */
    ~MicroBitAccessibility();

    private:

    /**
     * Event handler, called whenever compass calibration occurs.
     */
    void calibrationEvent(MicroBitEvent e);

    /**
     * Event handler, called whenever a text based animation (such as scroll, print, etc) is called on the display.
     */
    void animationEvent(MicroBitEvent e);

    /**
     * Snapshot of the last frame transmitted. Used to determine if a display has been updated.
     */
    MicroBitImage lastFrame;

    void start();
    void stop();
};

#endif
