/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Modifications Copyright (c) 2016 Calliope GbR
Modifications are provided by DELTA Systems (Georg Sommer) - Thomas Kern
und Björn Eberhardt GbR by arrangement with Calliope GbR. 

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
#define IO_STATUS_EVENTBUS_ENABLED          0x80        // Pin is will generate events on change

//#defines for each edge connector pin - changed!
#define MICROBIT_PIN_P0                     P0_3        //P0 is the left most pad (ANALOG/DIGITAL) used to be P0_3 on green board
#define MICROBIT_PIN_P1                     P0_2        //P1 is the middle pad (ANALOG/DIGITAL)
#define MICROBIT_PIN_P2                     P0_1        //P2 is the right most pad (ANALOG/DIGITAL) used to be P0_1 on green board
#define MICROBIT_PIN_P3                     P0_4        //COL1 (ANALOG/DIGITAL)
#define MICROBIT_PIN_P4                     P0_5        //COL2 (ANALOG/DIGITAL)
#define MICROBIT_PIN_P5                     P0_17       //BTN_A
#define MICROBIT_PIN_P6                     P0_12       //COL9
#define MICROBIT_PIN_P7                     P0_11       //COL8
#define MICROBIT_PIN_P9                     P0_10       //COL7
#define MICROBIT_PIN_P10                    P0_6        //COL3 (ANALOG/DIGITAL)
#define MICROBIT_PIN_P11                    P0_26       //BTN_B
#define MICROBIT_PIN_P19                    P0_19       //SCL
#define MICROBIT_PIN_P20                    P0_20       //SDA
//CALLIOPE MINI pins added here
#define CALLIOPE_PIN_P0                     P0_0        //pad P0 on Calliope Mini board
#define CALLIOPE_PIN_P7                     P0_7        //LED control / IO pin  
#define CALLIOPE_PIN_P8                     P0_8        //LED control / IO pin       
#define CALLIOPE_PIN_P9                     P0_9        //LED control / IO pin 
#define CALLIOPE_PIN_P13                    P0_13       //LED control / IO pin 
#define CALLIOPE_PIN_P14                    P0_14       //LED control / IO pin 
#define CALLIOPE_PIN_P15                    P0_15       //LED control / IO pin 

#define MICROBIT_PIN_MAX_OUTPUT             1023

#define MICROBIT_PIN_MAX_SERVO_RANGE        180
#define MICROBIT_PIN_DEFAULT_SERVO_RANGE    2000
#define MICROBIT_PIN_DEFAULT_SERVO_CENTER   1500


/**
  * Pin capabilities enum.
  * Used to determine the capabilities of each Pin as some can only be digital, or can be both digital and analogue.
  */
enum PinCapability{
    PIN_CAPABILITY_DIGITAL = 0x01,
    PIN_CAPABILITY_ANALOG = 0x02,
    PIN_CAPABILITY_TOUCH = 0x04,
    PIN_CAPABILITY_AD = PIN_CAPABILITY_DIGITAL | PIN_CAPABILITY_ANALOG,
    PIN_CAPABILITY_ALL = PIN_CAPABILITY_DIGITAL | PIN_CAPABILITY_ANALOG | PIN_CAPABILITY_TOUCH
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
      *                   (PIN_CAPABILITY_DIGITAL, PIN_CAPABILITY_ANALOG, PIN_CAPABILITY_TOUCH, PIN_CAPABILITY_AD, PIN_CAPABILITY_ALL)
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
      * @return 1 if this input is high, 0 if input is LO, or MICROBIT_NOT_SUPPORTED
      *         if the given pin does not have analog capability.
      *
      * @code
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * P0.getDigitalValue(); // P0 is either 0 or 1;
      * @endcode
      */
    int getDigitalValue();

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
};

#endif
