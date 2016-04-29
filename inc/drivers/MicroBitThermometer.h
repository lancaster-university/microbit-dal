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

#ifndef MICROBIT_THERMOMETER_H
#define MICROBIT_THERMOMETER_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "MicroBitStorage.h"

#define MICROBIT_THERMOMETER_PERIOD             1000


#define MAG3110_SAMPLE_RATES                    11

/*
 * Temperature events
 */
#define MICROBIT_THERMOMETER_EVT_UPDATE         1

#define MICROBIT_THERMOMETER_ADDED_TO_IDLE      2

/**
  * Class definition for MicroBit Thermometer.
  *
  * Infers and stores the ambient temoperature based on the surface temperature
  * of the various chips on the micro:bit.
  *
  */
class MicroBitThermometer : public MicroBitComponent
{
    unsigned long           sampleTime;
    uint32_t                samplePeriod;
    int16_t                 temperature;
    int16_t                 offset;
    MicroBitStorage*        storage;

    public:

    /**
      * Constructor.
      * Create new MicroBitThermometer that gives an indication of the current temperature.
      *
      * @param _storage an instance of MicroBitStorage used to persist temperature offset data
      *
      * @param id the unique EventModel id of this component. Defaults to MICROBIT_ID_THERMOMETER.
      *
      * @code
      * MicroBitStorage storage;
      * MicroBitThermometer thermometer(storage);
      * @endcode
      */
    MicroBitThermometer(MicroBitStorage& _storage, uint16_t id = MICROBIT_ID_THERMOMETER);

    /**
      * Constructor.
      * Create new MicroBitThermometer that gives an indication of the current temperature.
      *
      * @param id the unique EventModel id of this component. Defaults to MICROBIT_ID_THERMOMETER.
      *
      * @code
      * MicroBitThermometer thermometer;
      * @endcode
      */
    MicroBitThermometer(uint16_t id = MICROBIT_ID_THERMOMETER);

    /**
      * Set the sample rate at which the temperatureis read (in ms).
      *
      * The default sample period is 1 second.
      *
      * @param period the requested time between samples, in milliseconds.
      *
      * @note the temperature is always read in the background, and is only updated
      * when the processor is idle, or when the temperature is explicitly read.
      */
    void setPeriod(int period);

    /**
      * Reads the currently configured sample rate of the thermometer.
      *
      * @return The time between samples, in milliseconds.
      */
    int getPeriod();

    /**
      * Set the value that is used to offset the raw silicon temperature.
      *
      * @param offset the offset for the silicon temperature
      *
      * @return MICROBIT_OK on success
      */
    int setOffset(int offset);

    /**
      * Retreive the value that is used to offset the raw silicon temperature.
      *
      * @return the current offset.
      */
    int getOffset();

    /**
      * This member function fetches the raw silicon temperature, and calculates
      * the value used to offset the raw silicon temperature based on a given temperature.
      *
      * @param calibrationTemp the temperature used to calculate the raw silicon temperature
      * offset.
      *
      * @return MICROBIT_OK on success
      */
    int setCalibration(int calibrationTemp);

    /**
      * Gets the current temperature of the microbit.
      *
      * @return the current temperature, in degrees celsius.
      *
      * @code
      * thermometer.getTemperature();
      * @endcode
      */
    int getTemperature();

    /**
      * Updates the temperature sample of this instance of MicroBitThermometer
      * only if isSampleNeeded() indicates that an update is required.
      *
      * This call also will add the thermometer to fiber components to receive
      * periodic callbacks.
      *
      * @return MICROBIT_OK on success.
      */
    int updateSample();

    /**
      * Periodic callback from MicroBit idle thread.
      */
    virtual void idleTick();

    private:

    /**
      * Determines if we're due to take another temperature reading
      *
      * @return 1 if we're due to take a temperature reading, 0 otherwise.
      */
    int isSampleNeeded();
};

#endif
