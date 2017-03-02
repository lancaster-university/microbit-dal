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
  * Class definition for DynamicPwm.
  *
  * This class addresses a few issues found in the underlying libraries.
  * This provides the ability for a neat, clean swap between PWM channels.
  */

#include "MicroBitConfig.h"
#include "DynamicPwm.h"
#include "MicroBitPin.h"
#include "ErrorNo.h"

uint32_t DynamicPwm::period = MICROBIT_DEFAULT_PWM_PERIOD;

/**
  * An internal constructor used when allocating a new DynamicPwm instance.
  *
  * @param pin the name of the pin for the pwm to target
  *
  * @param persistance the level of persistence for this pin PWM_PERSISTENCE_PERSISTENT (can not be replaced until freed, should only be used for system services really.)
  *                    or PWM_PERSISTENCE_TRANSIENT (can be replaced at any point if a channel is required.)
  */
DynamicPwm::DynamicPwm(PinName pin) : PwmOut(pin)
{
}

/**
  * Frees this DynamicPwm instance for reuse.
  *
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate();
  * pwm->release();
  * @endcode
  */
DynamicPwm::~DynamicPwm()
{
    //free the pwm instance.
    pwmout_free(&_pwm);
}

/**
  * A lightweight wrapper around the super class' write in order to capture the value
  *
  * @param value the duty cycle percentage in floating point format.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range
  *
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate();
  * pwm->write(0.5);
  * @endcode
  */
int DynamicPwm::write(float value){

    if(value < 0)
        return MICROBIT_INVALID_PARAMETER;

    PwmOut::write(value);
    lastValue = value;

    return MICROBIT_OK;
}

/**
  * Retrieves the PinName associated with this DynamicPwm instance.
  *
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  *
  * // returns the PinName n.
  * pwm->getPinName();
  * @endcode
  *
  * @note This should be used to check that the DynamicPwm instance has not
  *       been reallocated for use in another part of a program.
  */
PinName DynamicPwm::getPinName()
{
    return _pwm.pin;
}

/**
  * Retrieves the last value that has been written to this DynamicPwm instance.
  * in the range 0 - 1023 inclusive.
  *
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->write(0.5);
  *
  * // will return 512.
  * pwm->getValue();
  * @endcode
  */
int DynamicPwm::getValue()
{
    return (float)lastValue * float(MICROBIT_PIN_MAX_OUTPUT);
}

/**
  * Retrieves the current period in use by the entire PWM module in microseconds.
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->getPeriod();
  * @endcode
  */
uint32_t DynamicPwm::getPeriodUs()
{
    return this->period;
}

/**
  * Retrieves the current period in use by the entire PWM module in milliseconds.
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  * pwm->setPeriodUs(20000);
  *
  * // will return 20000
  * pwm->getPeriod();
  * @endcode
  */
uint32_t DynamicPwm::getPeriod()
{
    return getPeriodUs() / 1000;
}

/**
  * Sets the period used by the WHOLE PWM module.
  *
  * @param period the desired period in microseconds.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if period is out of range
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  *
  * // period now is 20ms
  * pwm->setPeriodUs(20000);
  * @endcode
  *
  * @note Any changes to the period will AFFECT ALL CHANNELS.
  */
int DynamicPwm::setPeriodUs(uint32_t period)
{
    period_us(period);
    write(lastValue);

    this->period = period;

    return MICROBIT_OK;
}

/**
  * Sets the period used by the WHOLE PWM module. Any changes to the period will AFFECT ALL CHANNELS.
  *
  * @param period the desired period in milliseconds.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if period is out of range
  *
  * Example:
  * @code
  * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
  *
  * // period now is 20ms
  * pwm->setPeriod(20);
  * @endcode
  */
int DynamicPwm::setPeriod(uint32_t period)
{
    return setPeriodUs(period * 1000);
}
