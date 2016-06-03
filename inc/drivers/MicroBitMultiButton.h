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

#ifndef MICROBIT_MULTI_BUTTON_H
#define MICROBIT_MULTI_BUTTON_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitButton.h"
#include "EventModel.h"

#define MICROBIT_MULTI_BUTTON_STATE_1               0x01
#define MICROBIT_MULTI_BUTTON_STATE_2               0x02
#define MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_1      0x04
#define MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_2      0x08
#define MICROBIT_MULTI_BUTTON_SUPRESSED_1           0X10
#define MICROBIT_MULTI_BUTTON_SUPRESSED_2           0x20
#define MICROBIT_MULTI_BUTTON_ATTACHED              0x40

/**
  * Class definition for MicroBitMultiButton.
  *
  * Represents a virtual button, capable of reacting to simultaneous presses of two
  * other buttons.
  */
class MicroBitMultiButton : public MicroBitComponent
{
    uint16_t    button1;        // ID of the first button we're monitoring
    uint16_t    button2;        // ID of the second button we're monitoring
    MicroBitButtonEventConfiguration eventConfiguration;    // Do we want to generate high level event (clicks), or defer this to another service.

    /**
      * Retrieves the button id for the alternate button id given.
      *
      * @param b the id of the button whose state we would like to retrieve.
      *
      * @return the other sub button id.
      */
    uint16_t    otherSubButton(uint16_t b);

    /**
      * Determines if the given button id is marked as pressed.
      *
      * @param button the id of the button whose state we would like to retrieve.
      *
      * @return 1 if pressed, 0 if not.
      */
    int         isSubButtonPressed(uint16_t button);

    /**
      * Determines if the given button id is marked as held.
      *
      * @param button the id of the button whose state we would like to retrieve.
      *
      * @return 1 if held, 0 if not.
      */
    int         isSubButtonHeld(uint16_t button);

    /**
      * Determines if the given button id is marked as supressed.
      *
      * @param button the id of the button whose state we would like to retrieve.
      *
      * @return 1 if supressed, 0 if not.
      */
    int         isSubButtonSupressed(uint16_t button);

    /**
      * Configures the button pressed state for the given button id.
      *
      * @param button the id of the button whose state requires updating.
      *
      * @param value the value to set for this buttons state. (Transformed into a logical 0 or 1).
      */
    void        setButtonState(uint16_t button, int value);

    /**
      * Configures the button held state for the given button id.
      *
      * @param button the id of the button whose state requires updating.
      *
      * @param value the value to set for this buttons state. (Transformed into a logical 0 or 1).
      */
    void        setHoldState(uint16_t button, int value);

    /**
      * Configures the button suppressed state for the given button id.
      *
      * @param button the id of the button whose state requires updating.
      *
      * @param value the value to set for this buttons state. (Transformed into a logical 0 or 1).
      */
    void        setSupressedState(uint16_t button, int value);

    public:

    /**
      * Constructor.
      *
      * Create a representation of a virtual button, that generates events based upon the combination
      * of two given buttons.
      *
      * @param button1 the unique ID of the first button to watch.
      *
      * @param button2 the unique ID of the second button to watch.
      *
      * @param id the unique EventModel id of this MicroBitMultiButton instance.
      *
      * @code
      * multiButton(MICROBIT_ID_BUTTON_A, MICROBIT_ID_BUTTON_B, MICROBIT_ID_BUTTON_AB);
      * @endcode
      */
    MicroBitMultiButton(uint16_t button1, uint16_t button2, uint16_t id);

    /**
      * Tests if this MicroBitMultiButton instance is virtually pressed.
      *
      * @return 1 if both physical buttons are pressed simultaneously.
      *
      * @code
      * if(buttonAB.isPressed())
      *     display.scroll("Pressed!");
      * @endcode
      */
    int isPressed();

    /**
      * Changes the event configuration of this button to the given MicroBitButtonEventConfiguration.
      * All subsequent events generated by this button will then be informed by this configuration.
      *
      * @param config The new configuration for this button. Legal values are MICROBIT_BUTTON_ALL_EVENTS or MICROBIT_BUTTON_SIMPLE_EVENTS.
      *
      * @code
      * // Configure a button to generate all possible events.
      * buttonAB.setEventConfiguration(MICROBIT_BUTTON_ALL_EVENTS);
      *
      * // Configure a button to suppress MICROBIT_BUTTON_EVT_CLICK and MICROBIT_BUTTON_EVT_LONG_CLICK events.
      * buttonAB.setEventConfiguration(MICROBIT_BUTTON_SIMPLE_EVENTS);
      * @endcode
      */
    void setEventConfiguration(MicroBitButtonEventConfiguration config);

    private:

    /**
      * A member function that is invoked when any event is detected from the two
      * button IDs this MicrobitMultiButton instance was constructed with.
      *
      * @param evt the event received from the default EventModel.
      */
    void onButtonEvent(MicroBitEvent evt);
};

#endif
