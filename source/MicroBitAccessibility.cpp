/*
The MIT License (MIT)

Copyright (c) 2016 Lancaster University.

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
 * This class provides functionality to export the state of the LED matrix display
 * over a given serial port. The aim of this is to enable the use of accessbility tools that
 * can replace the LED matrix display with an alternate output device, such as braille.
 */

#include "MicroBit.h"

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
MicroBitAccessibility::MicroBitAccessibility(uint16_t id)
{
    // Store our identifiers.
    this->id = id;
    this->status = 0;
}

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
int MicroBitAccessibility::enable()
{
    MicroBitStorage s = MicroBitStorage();
    MicroBitConfigurationBlock *b = s.getConfigurationBlock();

    //check we are not storing our restored calibration data.
    if(b->magic != MICROBIT_STORAGE_CONFIG_MAGIC || b->accessibility != 1)
    {
        b->magic = MICROBIT_STORAGE_CONFIG_MAGIC;
        b->accessibility = 1;

        s.setConfigurationBlock(b);
    }

    delete b;

    status |= MICROBIT_ACCESSIBILITY_ENABLED;

    uBit.addIdleComponent(this);
    uBit.MessageBus.listen(MICROBIT_ID_DISPLAY, MICROBIT_EVT_ANY, this, &MicroBitAccessibility::animationEvent);
    uBit.MessageBus.listen(MICROBIT_ID_COMPASS, MICROBIT_EVT_ANY, this, &MicroBitAccessibility::calibrationEvent, MESSAGE_BUS_LISTENER_IMMEDIATE);

    return MICROBIT_OK;
}

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
int MicroBitAccessibility::disable()
{
    MicroBitStorage s = MicroBitStorage();
    MicroBitConfigurationBlock *b = s.getConfigurationBlock();

    //check we are not storing our restored calibration data.
    if(b->accessibility)
    {
        b->magic = MICROBIT_STORAGE_CONFIG_MAGIC;
        b->accessibility = 0;

        s.setConfigurationBlock(b);
    }

    delete b;

    status &= ~MICROBIT_ACCESSIBILITY_ENABLED;
    uBit.removeIdleComponent(this);
    uBit.MessageBus.ignore(MICROBIT_ID_DISPLAY, MICROBIT_EVT_ANY, this, &MicroBitAccessibility::animationEvent);
    uBit.MessageBus.ignore(MICROBIT_ID_COMPASS, MICROBIT_EVT_ANY, this, &MicroBitAccessibility::calibrationEvent);

    return MICROBIT_OK;
}

/**
 * Periodic callback. Check if htere have been any updates on the display. If so, post an update down the serial line.
 */
void MicroBitAccessibility::idleTick()
{
    MicroBitImage i = uBit.display.screenShot();

    // if there have been no updates to the display buffer, we have nothing to do.
    if (i == lastFrame)
        return;

    // Store a reference to the last frame
    lastFrame = i;

    // Stream out the values.
    if (uBit.display.getDisplayMode() == DISPLAY_MODE_GREYSCALE)
    {
        // Supply 8 bit data in ASCII HEX format
        uBit.serial.printf("{led256:\"");
        for (int y=0; y<lastFrame.getHeight(); y++)
        {
            for (int x=0; x<lastFrame.getWidth(); x++)
            {
                uBit.serial.printf("%.2X", lastFrame.getPixelValue(x,y));
                if (!(x == lastFrame.getWidth() - 1 && y == lastFrame.getHeight() - 1))
                    uBit.serial.printf(",");

            }
        }
        uBit.serial.printf("\"}\n");
    }
    else
    {
        // Supply 1 bit data in ASCII HEX format
        uBit.serial.printf("{led:\"");
        uint32_t data;

        for (int y=0; y<lastFrame.getHeight(); y++)
        {
            data = 0;
            for (int x=0; x<lastFrame.getWidth(); x++)
            {
                if (lastFrame.getPixelValue(x,y))
                    data |= 1 << ((lastFrame.getWidth() - x ) - 1);
            }

            uBit.serial.printf("%.2X", data);
            if (y != lastFrame.getHeight() - 1)
                uBit.serial.printf(",");

        }
        uBit.serial.printf("\"}\n");
    }
}

/**
 * Event handler, called whenever compass calibration occurs.
 */
void MicroBitAccessibility::calibrationEvent(MicroBitEvent e)
{
    if (e.value == MICROBIT_COMPASS_EVT_CALIBRATE)
        uBit.serial.printf("{compass-calibrating: 1}\n");

    if (e.value == MICROBIT_COMPASS_EVT_CALIBRATE_COMPLETE)
        uBit.serial.printf("{compass-calibrating: 0}\n");
}

/**
 * Event handler, called whenever a text based animation (such as scroll, print, etc) is called on the display.
 */
void MicroBitAccessibility::animationEvent(MicroBitEvent e)
{
    if (e.value == MICROBIT_DISPLAY_EVT_ANIMATION_STARTED)
    {
        ManagedString s = uBit.display.getMessage();

        // If the animation is not textual, then there's nothing for us to do.
        if (s == ManagedString::EmptyString)
            return;

        // Transmit the text being animated. 
        AnimationMode a = uBit.display.getAnimationMode();

        if (a == ANIMATION_MODE_SCROLL_TEXT)
        {
            uBit.serial.printf("{scroll:\"%s\"}\n", s.toCharArray());
            return;
        }

        if (a == ANIMATION_MODE_PRINT_TEXT)
        {
            uBit.serial.printf("{print:\"%s\"}\n", s.toCharArray());
            return;
        }
    }

    if (e.value == MICROBIT_DISPLAY_EVT_ANIMATION_STOPPED)
    {
        uBit.serial.printf("{stop:\"\"}\n");
        return;
    }
}

/**
  * Destructor for MicroBitAccessibility, so that we deregister ourselves as an idleComponent
  */
MicroBitAccessibility::~MicroBitAccessibility()
{
    uBit.removeIdleComponent(this);
    uBit.MessageBus.ignore(MICROBIT_ID_DISPLAY, MICROBIT_EVT_ANY, this, &MicroBitAccessibility::animationEvent);
}

