#include "MicroBit.h"
#include "MicroBitPin.h"

/**
  * Constructor.
  * Create a Button representation with the given ID.
  * @param id the ID of the new Pin object.
  * @param name the pin name for this MicroBitPin instance to represent
  * @param capability the capability of this pin, can it only be digital? can it only be analog? can it be both?
  *
  * Example:
  * @code
  * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
  * @endcode
  */
MicroBitPin::MicroBitPin(int id, PinName name, PinCapability capability)
{
    //set mandatory attributes
    this->id = id;
    this->name = name;
    this->capability = capability;

    // Power up in a disconnected, low power state.
    // If we're unused, this is how it will stay...
    this->status = 0x00;
    this->pin = NULL;

}

/**
  * Disconnect any attached mbed IO from this pin.
  * Used only when pin changes mode (i.e. Input/Output/Analog/Digital)
  */
void MicroBitPin::disconnect()
{
    // This is a bit ugly, but rarely used code.
    // It would be much better to use some polymorphism here, but the mBed I/O classes aren't arranged in an inheritance hierarchy... yet. :-)
    if (status & IO_STATUS_DIGITAL_IN)
        delete ((DigitalIn *)pin);

    if (status & IO_STATUS_DIGITAL_OUT)
        delete ((DigitalOut *)pin);

    if (status & IO_STATUS_ANALOG_IN){
        NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Disabled; // forcibly disable the ADC - BUG in mbed....
        delete ((AnalogIn *)pin);
    }

    if (status & IO_STATUS_ANALOG_OUT)
    {
        if(((DynamicPwm *)pin)->getPinName() == name)
            ((DynamicPwm *)pin)->release();
    }

    if (status & IO_STATUS_TOUCH_IN)
        delete ((MicroBitButton *)pin);

    this->pin = NULL;
    this->status = status & IO_STATUS_EVENTBUS_ENABLED; //retain event bus status
}

/**
  * Configures this IO pin as a digital output (if necessary) and sets the pin to 'value'.
  * @param value 0 (LO) or 1 (HI)
  *
  * Example:
  * @code
  * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
  * P0.setDigitalValue(1); // P0 is now HI
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range, or MICROBIT_NOT_SUPPORTED
  * if the given pin does not have digital capability.
  * @endcode
  */
int MicroBitPin::setDigitalValue(int value)
{
    // Check if this pin has a digital mode...
    if(!(PIN_CAPABILITY_DIGITAL & capability))
        return MICROBIT_NOT_SUPPORTED;

    // Ensure we have a valid value.
    if (value < 0 || value > 1)
        return MICROBIT_INVALID_PARAMETER;

    // Move into a Digital input state if necessary.
    if (!(status & IO_STATUS_DIGITAL_OUT)){
        disconnect();
        pin = new DigitalOut(name);
        status |= IO_STATUS_DIGITAL_OUT;
    }

    // Write the value.
    ((DigitalOut *)pin)->write(value);

    return MICROBIT_OK;
}

/**
  * Configures this IO pin as a digital input (if necessary) and tests its current value.
  * @return 1 if this input is high, 0 if input is LO, or MICROBIT_NOT_SUPPORTED if the given pin does not have analog capability.
  *
  * Example:
  * @code
  * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
  * P0.getDigitalValue(); // P0 is either 0 or 1;
  * @endcode
  */
int MicroBitPin::getDigitalValue()
{
    //check if this pin has a digital mode...
    if(!(PIN_CAPABILITY_DIGITAL & capability))
        return MICROBIT_NOT_SUPPORTED;

    // Move into a Digital input state if necessary.
    if (!(status & IO_STATUS_DIGITAL_IN)){
        disconnect();
        pin = new DigitalIn(name,PullDown);
        status |= IO_STATUS_DIGITAL_IN;
    }

    return ((DigitalIn *)pin)->read();
}

/**
 * Configures this IO pin as an analog/pwm output, and change the output value to the given level.
 * @param value the level to set on the output pin, in the range 0 - 1024
 * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range, or MICROBIT_NOT_SUPPORTED
 * if the given pin does not have analog capability.
 */
int MicroBitPin::setAnalogValue(int value)
{
    //check if this pin has an analogue mode...
    if(!(PIN_CAPABILITY_ANALOG & capability))
        return MICROBIT_NOT_SUPPORTED;

    //sanitise the brightness level
    if(value < 0 || value > MICROBIT_PIN_MAX_OUTPUT)
        return MICROBIT_INVALID_PARAMETER;

    float level = (float)value / float(MICROBIT_PIN_MAX_OUTPUT);

    // Move into an analogue input state if necessary, if we are no longer the focus of a DynamicPWM instance, allocate ourselves again!
    if (!(status & IO_STATUS_ANALOG_OUT) || !(((DynamicPwm *)pin)->getPinName() == name)){
        disconnect();
        pin = (void *)DynamicPwm::allocate(name);
        status |= IO_STATUS_ANALOG_OUT;
    }

    //perform a write with an extra check! :)
    if(((DynamicPwm *)pin)->getPinName() == name)
        ((DynamicPwm *)pin)->write(level);

    return MICROBIT_OK;
}


/**
 * Configures this IO pin as an analogue input (if necessary and possible).
 * @return the current analogue level on the pin, in the range 0 - 1024, or MICROBIT_NOT_SUPPORTED if the given pin does not have analog capability.
 *
 * Example:
 * @code
 * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
 * P0.getAnalogValue(); // P0 is a value in the range of 0 - 1024
 * @endcode
 */
int MicroBitPin::getAnalogValue()
{
    //check if this pin has an analogue mode...
    if(!(PIN_CAPABILITY_ANALOG & capability))
        return MICROBIT_NOT_SUPPORTED;

    // Move into an analogue input state if necessary.
    if (!(status & IO_STATUS_ANALOG_IN)){
        disconnect();
        pin = new AnalogIn(name);
        status |= IO_STATUS_ANALOG_IN;
    }

    //perform a read!
    return ((AnalogIn *)pin)->read_u16();
}

/**
  * Determines if this IO pin is currently configured as an input.
  * @return 1 if pin is an analog or digital input, 0 otherwise.
  */
int MicroBitPin::isInput()
{
    return (status & (IO_STATUS_DIGITAL_IN | IO_STATUS_ANALOG_IN)) == 0 ? 0 : 1;
}

/**
  * Determines if this IO pin is currently configured as an output.
  * @return 1 if pin is an analog or digital output, 0 otherwise.
  */
int MicroBitPin::isOutput()
{
    return (status & (IO_STATUS_DIGITAL_OUT | IO_STATUS_ANALOG_OUT)) == 0 ? 0 : 1;
}

/**
  * Determines if this IO pin is currently configured for digital use.
  * @return 1 if pin is digital, 0 otherwise.
  */
int MicroBitPin::isDigital()
{
    return (status & (IO_STATUS_DIGITAL_IN | IO_STATUS_DIGITAL_OUT)) == 0 ? 0 : 1;
}

/**
  * Determines if this IO pin is currently configured for analog use.
  * @return 1 if pin is analog, 0 otherwise.
  */
int MicroBitPin::isAnalog()
{
    return (status & (IO_STATUS_ANALOG_IN | IO_STATUS_ANALOG_OUT)) == 0 ? 0 : 1;
}

/**
  * Configures this IO pin as a makey makey style touch sensor (if necessary) and tests its current debounced state.
  * @return 1 if pin is touched, 0 if not, or MICROBIT_NOT_SUPPORTED if this pin does not support touch capability.
  *
  * Example:
  * @code
  * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL);
  * if(P0.isTouched())
  * {
  *   uBit.display.clear();
  * }
  * @endcode
  */
int MicroBitPin::isTouched()
{
    //check if this pin has a touch mode...
    if(!(PIN_CAPABILITY_TOUCH & capability))
        return MICROBIT_NOT_SUPPORTED;

    // Move into a touch input state if necessary.
    if (!(status & IO_STATUS_TOUCH_IN)){
        disconnect();
        pin = new MicroBitButton(id, name);
        status |= IO_STATUS_TOUCH_IN;
    }

    return ((MicroBitButton *)pin)->isPressed();
}

/**
  * Configures the PWM period of the analog output to the given value.
  *
  * @param period The new period for the analog output in microseconds.
  * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the
  * given pin is not configured as an analog output.
  */
int MicroBitPin::setAnalogPeriodUs(int period)
{
    if (!(status & IO_STATUS_ANALOG_OUT))
        return MICROBIT_NOT_SUPPORTED;

    ((DynamicPwm *)pin)->setPeriodUs(period);
    return MICROBIT_OK;
}

/**
 * Configures the PWM period of the analog output to the given value.
 *
 * @param period The new period for the analog output in microseconds.
 * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the
 * given pin is not configured as an analog output.
 */
int MicroBitPin::setAnalogPeriod(int period)
{
    return setAnalogPeriodUs(period*1000);
}
