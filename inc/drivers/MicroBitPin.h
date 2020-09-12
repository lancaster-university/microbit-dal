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

#ifndef MICROBIT_PIN_H
#define MICROBIT_PIN_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
                                                        // Status Field flags...
#define IO_STATUS_DIGITAL_IN                0x01        // Pin is configured as a digital input, with no pull up.
#define IO_STATUS_DIGITAL_OUT               0x02        // Pin is configured as a digital output
#define IO_STATUS_ANALOG_IN                 0x04        // Pin is Analog in
#define IO_STATUS_ANALOG_OUT                0x08        // Pin is Analog out
#define IO_STATUS_TOUCH_IN                  0x10        // Pin is a makey-makey style touch sensor
#define IO_STATUS_EVENT_ON_EDGE             0x20        // Pin will generate events on pin change
#define IO_STATUS_EVENT_PULSE_ON_EDGE       0x40        // Pin will generate events on pin change

//#defines for each edge connector pin
#define MICROBIT_PIN_P0                     P0_3        //P0 is the left most pad (ANALOG/DIGITAL) used to be P0_3 on green board
#define MICROBIT_PIN_P1                     P0_2        //P1 is the middle pad (ANALOG/DIGITAL)
#define MICROBIT_PIN_P2                     P0_1        //P2 is the right most pad (ANALOG/DIGITAL) used to be P0_1 on green board
#define MICROBIT_PIN_P3                     P0_4        //COL1 (ANALOG/DIGITAL)
#define MICROBIT_PIN_P4                     P0_5        //COL2 (ANALOG/DIGITAL)
#define MICROBIT_PIN_P5                     P0_17       //BTN_A
#define MICROBIT_PIN_P6                     P0_12       //COL9
#define MICROBIT_PIN_P7                     P0_11       //COL8
#define MICROBIT_PIN_P8                     P0_18       //PIN 18
#define MICROBIT_PIN_P9                     P0_10       //COL7
#define MICROBIT_PIN_P10                    P0_6        //COL3 (ANALOG/DIGITAL)
#define MICROBIT_PIN_P11                    P0_26       //BTN_B
#define MICROBIT_PIN_P12                    P0_20       //PIN 20
#define MICROBIT_PIN_P13                    P0_23       //SCK
#define MICROBIT_PIN_P14                    P0_22       //MISO
#define MICROBIT_PIN_P15                    P0_21       //MOSI
#define MICROBIT_PIN_P16                    P0_16       //PIN 16
#define MICROBIT_PIN_P19                    P0_0        //SCL
#define MICROBIT_PIN_P20                    P0_30       //SDA

#define MICROBIT_PIN_MAX_OUTPUT             1023

#define MICROBIT_PIN_MAX_SERVO_RANGE        180
#define MICROBIT_PIN_DEFAULT_SERVO_RANGE    2000
#define MICROBIT_PIN_DEFAULT_SERVO_CENTER   1500

#define MICROBIT_PIN_EVENT_NONE             0
#define MICROBIT_PIN_EVENT_ON_EDGE          2
#define MICROBIT_PIN_EVENT_ON_PULSE         3
#define MICROBIT_PIN_EVENT_ON_TOUCH         4

#define MICROBIT_PIN_EVT_RISE               2
#define MICROBIT_PIN_EVT_FALL               3
#define MICROBIT_PIN_EVT_PULSE_HI           4
#define MICROBIT_PIN_EVT_PULSE_LO           5

/**
  * Pin capabilities enum.
  * Used to determine the capabilities of each Pin as some can only be digital, or can be both digital and analogue.
  */
enum PinCapability{
    PIN_CAPABILITY_DIGITAL_IN = 0x01,
    PIN_CAPABILITY_DIGITAL_OUT = 0x02,
    PIN_CAPABILITY_DIGITAL = PIN_CAPABILITY_DIGITAL_IN | PIN_CAPABILITY_DIGITAL_OUT,
    PIN_CAPABILITY_ANALOG_IN = 0x04,
    PIN_CAPABILITY_ANALOG_OUT = 0x08,
    PIN_CAPABILITY_ANALOG = PIN_CAPABILITY_ANALOG_IN | PIN_CAPABILITY_ANALOG_OUT,
    PIN_CAPABILITY_STANDARD = PIN_CAPABILITY_DIGITAL | PIN_CAPABILITY_ANALOG_OUT,
    PIN_CAPABILITY_ALL = PIN_CAPABILITY_DIGITAL | PIN_CAPABILITY_ANALOG
};

/**
  * Class definition for MicroBitPin.
  *
  * Commonly represents an I/O pin on the edge connector.
  */
class MicroBitPin : public MicroBitComponent
{
    // The mbed object looking after this pin at any point in time (untyped due to dynamic behaviour).
    void *pin;
    PinCapability capability;
    uint8_t pullMode;

    /**
      * Disconnect any attached mBed IO from this pin.
      *
      * Used only when pin changes mode (i.e. Input/Output/Analog/Digital)
      */
    void disconnect();

    /**
      * Performs a check to ensure that the current Pin is in control of a
      * DynamicPwm instance, and if it's not, allocates a new DynamicPwm instance.
      */
    int obtainAnalogChannel();

    /**
      * Interrupt handler for when an rise interrupt is triggered.
      */
    void onRise();

    /**
      * Interrupt handler for when an fall interrupt is triggered.
      */
    void onFall();

    /**
      * This member function manages the calculation of the timestamp of a pulse detected
      * on a pin whilst in IO_STATUS_EVENT_PULSE_ON_EDGE or IO_STATUS_EVENT_ON_EDGE modes.
      *
      * @param eventValue the event value to distribute onto the message bus.
      */
    void pulseWidthEvent(int eventValue);

    /**
      * This member function will construct an TimedInterruptIn instance, and configure
      * interrupts for rise and fall.
      *
      * @param eventType the specific mode used in interrupt context to determine how an
      *                  edge/rise is processed.
      *
      * @return MICROBIT_OK on success
      */
    int enableRiseFallEvents(int eventType);

    /**
      * If this pin is in a mode where the pin is generating events, it will destruct
      * the current instance attached to this MicroBitPin instance.
      *
      * @return MICROBIT_OK on success.
      */
    int disableEvents();

    public:

    // mbed PinName of this pin.
    PinName name;

    /**
      * Constructor.
      * Create a MicroBitPin instance, generally used to represent a pin on the edge connector.
      *
      * @param id the unique EventModel id of this component.
      *
      * @param name the mbed PinName for this MicroBitPin instance.
      *
      * @param capability the capabilities this MicroBitPin instance should have.
      *                   (PIN_CAPABILITY_DIGITAL, PIN_CAPABILITY_ANALOG, PIN_CAPABILITY_AD, PIN_CAPABILITY_ALL)
      *
      * @code
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL);
      * @endcode
      */
    MicroBitPin(int id, PinName name, PinCapability capability);

    /**
      * Configures this IO pin as a digital output (if necessary) and sets the pin to 'value'.
      *
      * @param value 0 (LO) or 1 (HI)
      *
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range, or MICROBIT_NOT_SUPPORTED
      *         if the given pin does not have digital capability.
      *
      * @code
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * P0.setDigitalValue(1); // P0 is now HI
      * @endcode
      */
    int setDigitalValue(int value);

    /**
      * Configures this IO pin as a digital input (if necessary) and tests its current value.
      *
      *
      * @return 1 if this input is high, 0 if input is LO, or MICROBIT_NOT_SUPPORTED
      *         if the given pin does not have digital capability.
      *
      * @code
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * P0.getDigitalValue(); // P0 is either 0 or 1;
      * @endcode
      */
    int getDigitalValue();

    /**
      * Configures this IO pin as a digital input with the specified internal pull-up/pull-down configuraiton (if necessary) and tests its current value.
      *
      * @param pull one of the mbed pull configurations: PullUp, PullDown, PullNone
      *
      * @return 1 if this input is high, 0 if input is LO, or MICROBIT_NOT_SUPPORTED
      *         if the given pin does not have digital capability.
      *
      * @code
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * P0.getDigitalValue(PullUp); // P0 is either 0 or 1;
      * @endcode
      */
    int getDigitalValue(PinMode pull);

    /**
      * Configures this IO pin as an analog/pwm output, and change the output value to the given level.
      *
      * @param value the level to set on the output pin, in the range 0 - 1024
      *
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range, or MICROBIT_NOT_SUPPORTED
      *         if the given pin does not have analog capability.
      */
    int setAnalogValue(int value);

    /**
      * Configures this IO pin as an analog/pwm output (if necessary) and configures the period to be 20ms,
      * with a duty cycle between 500 us and 2500 us.
      *
      * A value of 180 sets the duty cycle to be 2500us, and a value of 0 sets the duty cycle to be 500us by default.
      *
      * This range can be modified to fine tune, and also tolerate different servos.
      *
      * @param value the level to set on the output pin, in the range 0 - 180.
      *
      * @param range which gives the span of possible values the i.e. the lower and upper bounds (center +/- range/2). Defaults to MICROBIT_PIN_DEFAULT_SERVO_RANGE.
      *
      * @param center the center point from which to calculate the lower and upper bounds. Defaults to MICROBIT_PIN_DEFAULT_SERVO_CENTER
      *
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range, or MICROBIT_NOT_SUPPORTED
      *         if the given pin does not have analog capability.
      */
    int setServoValue(int value, int range = MICROBIT_PIN_DEFAULT_SERVO_RANGE, int center = MICROBIT_PIN_DEFAULT_SERVO_CENTER);

    /**
      * Configures this IO pin as an analogue input (if necessary), and samples the Pin for its analog value.
      *
      * @return the current analogue level on the pin, in the range 0 - 1024, or
      *         MICROBIT_NOT_SUPPORTED if the given pin does not have analog capability.
      *
      * @code
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * P0.getAnalogValue(); // P0 is a value in the range of 0 - 1024
      * @endcode
      */
    int getAnalogValue();

    /**
      * Determines if this IO pin is currently configured as an input.
      *
      * @return 1 if pin is an analog or digital input, 0 otherwise.
      */
    int isInput();

    /**
      * Determines if this IO pin is currently configured as an output.
      *
      * @return 1 if pin is an analog or digital output, 0 otherwise.
      */
    int isOutput();

    /**
      * Determines if this IO pin is currently configured for digital use.
      *
      * @return 1 if pin is digital, 0 otherwise.
      */
    int isDigital();

    /**
      * Determines if this IO pin is currently configured for analog use.
      *
      * @return 1 if pin is analog, 0 otherwise.
      */
    int isAnalog();

    /**
      * Configures this IO pin as a "makey makey" style touch sensor (if necessary)
      * and tests its current debounced state.
      *
      * Users can also subscribe to MicroBitButton events generated from this pin.
      *
      * @return 1 if pin is touched, 0 if not, or MICROBIT_NOT_SUPPORTED if this pin does not support touch capability.
      *
      * @code
      * MicroBitMessageBus bus;
      *
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL);
      * if(P0.isTouched())
      * {
      *     //do something!
      * }
      *
      * // subscribe to events generated by this pin!
      * bus.listen(MICROBIT_ID_IO_P0, MICROBIT_BUTTON_EVT_CLICK, someFunction);
      * @endcode
      */
    int isTouched();

    /**
      * Configures this IO pin as an analog/pwm output if it isn't already, configures the period to be 20ms,
      * and sets the pulse width, based on the value it is given.
      *
      * @param pulseWidth the desired pulse width in microseconds.
      *
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range, or MICROBIT_NOT_SUPPORTED
      *         if the given pin does not have analog capability.
      */
    int setServoPulseUs(int pulseWidth);

    /**
      * Configures the PWM period of the analog output to the given value.
      *
      * @param period The new period for the analog output in milliseconds.
      *
      * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the
      *         given pin is not configured as an analog output.
      */
    int setAnalogPeriod(int period);

    /**
      * Configures the PWM period of the analog output to the given value.
      *
      * @param period The new period for the analog output in microseconds.
      *
      * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the
      *         given pin is not configured as an analog output.
      */
    int setAnalogPeriodUs(int period);

    /**
      * Obtains the PWM period of the analog output in microseconds.
      *
      * @return the period on success, or MICROBIT_NOT_SUPPORTED if the
      *         given pin is not configured as an analog output.
      */
    int getAnalogPeriodUs();

    /**
      * Obtains the PWM period of the analog output in milliseconds.
      *
      * @return the period on success, or MICROBIT_NOT_SUPPORTED if the
      *         given pin is not configured as an analog output.
      */
    int getAnalogPeriod();

    /**
      * Configures the pull of this pin.
      *
      * @param pull one of the mbed pull configurations: PullUp, PullDown, PullNone
      *
      * @return MICROBIT_NOT_SUPPORTED if the current pin configuration is anything other
      *         than a digital input, otherwise MICROBIT_OK.
      */
    int setPull(PinMode pull);

    /**
      * Configures the events generated by this MicroBitPin instance.
      *
      * MICROBIT_PIN_EVENT_ON_EDGE - Configures this pin to a digital input, and generates events whenever a rise/fall is detected on this pin. (MICROBIT_PIN_EVT_RISE, MICROBIT_PIN_EVT_FALL)
      * MICROBIT_PIN_EVENT_ON_PULSE - Configures this pin to a digital input, and generates events where the timestamp is the duration that this pin was either HI or LO. (MICROBIT_PIN_EVT_PULSE_HI, MICROBIT_PIN_EVT_PULSE_LO)
      * MICROBIT_PIN_EVENT_ON_TOUCH - Configures this pin as a makey makey style touch sensor, in the form of a MicroBitButton. Normal button events will be generated using the ID of this pin.
      * MICROBIT_PIN_EVENT_NONE - Disables events for this pin.
      *
      * @param eventType One of: MICROBIT_PIN_EVENT_ON_EDGE, MICROBIT_PIN_EVENT_ON_PULSE, MICROBIT_PIN_EVENT_ON_TOUCH, MICROBIT_PIN_EVENT_NONE
      *
      * @code
      * MicroBitMessageBus bus;
      *
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * P0.eventOn(MICROBIT_PIN_EVENT_ON_PULSE);
      *
      * void onPulse(MicroBitEvent evt)
      * {
      *     int duration = evt.timestamp;
      * }
      *
      * bus.listen(MICROBIT_ID_IO_P0, MICROBIT_PIN_EVT_PULSE_HI, onPulse, MESSAGE_BUS_LISTENER_IMMEDIATE)
      * @endcode
      *
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the given eventype does not match
      *
      * @note In the MICROBIT_PIN_EVENT_ON_PULSE mode, the smallest pulse that was reliably detected was 85us, around 5khz. If more precision is required,
      *       please use the InterruptIn class supplied by ARM mbed.
      */
    int eventOn(int eventType);
};

#endif
