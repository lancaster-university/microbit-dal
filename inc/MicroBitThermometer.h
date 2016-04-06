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
#include "MicroBitComponent.h"

#define MICROBIT_THERMOMETER_PERIOD             1000


#define MAG3110_SAMPLE_RATES                    11 

/*
 * Temperature events
 */
#define MICROBIT_THERMOMETER_EVT_UPDATE         1

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

    public:
    /**
     * Constructor. 
     * Create new object that can sense temperature.
     * @param id the ID of the new MicroBitThermometer object.
     *
     * Example:
     * @code 
     * thermometer(MICROBIT_ID_THERMOMETER); 
     * @endcode
     *
     * Possible Events:
     * @code 
     * MICROBIT_THERMOMETER_EVT_CHANGED
     * @endcode
     */
    MicroBitThermometer(uint16_t id);

    /**
     * Set the sample rate at which the temperatureis read (in ms).
     * n.b. the temperature is alwasy read in the background, so wis only updated
     * when the processor is idle, or when the temperature is explicitly read. 
     * The default sample period is 1 second.
     * @param period the requested time between samples, in milliseconds.
     */
    void setPeriod(int period);

    /**
      * Reads the currently configured sample rate of the thermometer. 
      * @return The time between samples, in milliseconds.
      */
    int getPeriod();

    /**
      * Gets the current temperature of the microbit.
      * @return the current temperature, in degrees celsius.
      *
      * Example:
      * @code
      * uBit.thermometer.getTemperature();
      * @endcode
      */
    int getTemperature();
    
    /**
      * Periodic callback from MicroBit idle thread.
      * Check if any data is ready for reading by checking the interrupt.
      */  
    virtual void idleTick();
    
    /**
      * Indicates if we'd like some processor time to sense the temperature. 0 means we're not due to read the tmeperature yet.
      * @returns 1 if we'd like some processor time, 0 otherwise.
      */
    virtual int isIdleCallbackNeeded();

    private:

    /**
      * Determines if we're due to take another temeoratur reading
      * @return 1 if we're due to take a temperature reading, 0 otherwise.
      */
    int isSampleNeeded();

    /**
      * Updates our recorded temeprature from the many sensors on the micro:bit!
      */
    void updateTemperature();

    
};

#endif
