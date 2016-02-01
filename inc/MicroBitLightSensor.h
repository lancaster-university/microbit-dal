#ifndef MICROBIT_LIGHT_SENSOR_H
#define MICROBIT_LIGHT_SENSOR_H

#include "mbed.h"
#include "MicroBitComponent.h"

#define MICROBIT_LIGHT_SENSOR_CHAN_NUM      3
#define MICROBIT_LIGHT_SENSOR_AN_SET_TIME   4000
#define MICROBIT_LIGHT_SENSOR_TICK_PERIOD   5

#define MICROBIT_LIGHT_SENSOR_MAX_VALUE     338
#define MICROBIT_LIGHT_SENSOR_MIN_VALUE     75

/**
  * Class definition for MicroBitLightSensor.
  *
  * This is an object that interleaves light sensing with uBit.display.
  */
class MicroBitLightSensor
{

    //contains the results from each section of the display
    int results[MICROBIT_LIGHT_SENSOR_CHAN_NUM] = { 0 };

    //holds the current channel (also used to index the results array)
    uint8_t chan;

    //a Timeout which triggers our analogReady() call
    Timeout analogTrigger;

    //a pointer the currently sensed pin, represented as an AnalogIn
    AnalogIn* sensePin;

    /**
      * After the startSensing method has been called, this method will be called
      * MICROBIT_LIGHT_SENSOR_AN_SET_TIME after.
      *
      * It will then read from the currently selected channel using the AnalogIn
      * that was configured in the startSensing method.
      */
    void analogReady();

    /**
      * Forcibly disables the AnalogIn, otherwise it will remain in possession
      * of the GPIO channel it is using, meaning that the display will not be
      * able to use a channel (COL).
      *
      * This is required as per PAN 3, details of which can be found here:
      *
      * https://www.nordicsemi.com/eng/nordic/download_resource/24634/5/88440387
      */
    void analogDisable();

    /**
      * The method that is invoked by sending MICROBIT_DISPLAY_EVT_LIGHT_SENSE
      * using the id MICROBIT_ID_DISPLAY.
      *
      * If you want to manually trigger this method, you should use the event bus.
      */
    void startSensing(MicroBitEvent);

    public:

    /**
      * Constructor.
      * Create a representation of the light sensor
      */
    MicroBitLightSensor();

    /**
      * This method returns a summed average of the three sections of the display.
      *
      * A section is defined as:
      *  ___________________
      * | 1 |   | 2 |   | 3 |
      * |___|___|___|___|___|
      * |   |   |   |   |   |
      * |___|___|___|___|___|
      * | 2 |   | 3 |   | 1 |
      * |___|___|___|___|___|
      * |   |   |   |   |   |
      * |___|___|___|___|___|
      * | 3 |   | 1 |   | 2 |
      * |___|___|___|___|___|
      *
      * Where each number represents a different section on the 5 x 5 matrix display.
      *
      * @return returns a value in the range 0 - 255 where 0 is dark, and 255
      * is very bright
      *
      * @note currently returns a value in the range 0 - 255 where 0 is dark, and 255
      * is very bright perhaps we should normalise the returned values into an SI unit!
      * TODO.
      */
    int read();

    /**
      * The destructor restores the default Display Mode and tick speed, and also
      * removes the listener from the MessageBus.
      */
    ~MicroBitLightSensor();
};

#endif
