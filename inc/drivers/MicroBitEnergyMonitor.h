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

#ifndef ENERGY_MONITOR_H
#define ENERGY_MONITOR_H

#include "MicroBitConfig.h"
#include "MicroBitCompass.h"
#include "MicroBitComponent.h"
#include "MicroBitEvent.h"
#include "MicroBitSystemTimer.h"
#include "MicroBitFiber.h"


#define SAMPLES                                    250

#define RANGE_MIN                                  4000
#define RANGE_MAX                                  160000
#define WATTAGE_MAX                                3000

#define MICROBIT_EVT_ELECTRICAL_POWER_EVT_ON       1
#define MICROBIT_EVT_ELECTRICAL_POWER_EVT_OFF      2

#define MICROBIT_ELECTRICAL_POWER_STATE            1

// @author: Taylor Woodcock
class MicroBitEnergyMonitor : public MicroBitComponent
{
    protected:

        int minFieldStrength;                // min sample of the magnetic field strength
        int maxFieldStrength;                // max sample of the magnetic field strength
        int sample;                          // the index of the current sample

        int watts;                           // current electrical power usage in watts
        int amplitude;                       // debug variable for measuring the amplitude of the samples

        MicroBitCompass& magnetometer;       // the magnetometer used for current sensing

    public:

        /**
          * Constructor that initialises the micro:bit magnetometer, the default status, and initialises the idleTick.
          *
          * @param magnetometer an instance of the micro:bit's magnetometer component
          *
          * @code
          * energyMonitor();
          * @endcode
          */
        MicroBitEnergyMonitor(MicroBitCompass &magnetometer, uint16_t id = MICROBIT_ID_ENERGY_MONITOR);

        /**
          * Periodic callback from MicroBit idle thread.
          */
        virtual void idleTick();

        /**
          *
          * Records one sample from the magnetometer and checks for state changes of the electrical power
          * when a set amount of samples have been gathered and fires various events on a state change.
          *
          */
        int updateSamples();

        /**
          * Tests the electrical power is currently on.
          *
          * @code
          * if(energyMonitor.isElectricalPowerOn())
          *     display.scroll("Power On!");
          * @endcode
          *
          * @return 1 if the electrical power is on, 0 otherwise.
          */
        int isElectricalPowerOn();

        /**
          * Takes samples of the amplitude of the electomagnetic field values
          * read using the magnetometer and returns the mapped value in Watts.
          *
          * @code
          * serial.send(getEnergyUsage());
          * @endcode
          *
          * @return the amount of electrical power being detected in Watts.
          */
        int getEnergyUsage();

        /**
          * Calibrates the current sensing values.
          */
        void calibrate();

        /**
          * Used for debug purposes for sampling the amplitude of the magnetometer samples.
          *
          * @returns the amplitude of the current sample set
          */
        int getAmplitude();

        /**
          * Destructor for MicroBitEnergyMonitor, where we deregister this instance from the array of fiber components.
          */
        ~MicroBitEnergyMonitor();

    private:

        /**
          * Maps a value from one range to another and returns 0 if the value is less than 0.
          *
          * @param value the value to be mapped from the old range to the new range
          * @param fromLow the low value from the original range
          * @param fromHigh the high value from the original range
          * @param toLow the low value of the new range
          * @param toHigh the high value of the new range
          *
          * @code
          * map(10, 1, 10, 1, 100);
          * // returns 100
          * @endcode
          *
          * @return the new value within the new range if the value is greater
          * than 0, or 0 if the value is less than or equal to 0.
          */
        int map(int value, int fromLow, int fromHigh, int toLow, int toHigh);

        /**
          * Approaches a goal value by incrementing the current value by delta.
          *
          * @param goal the goal value to approach
          * @param current the current value being incremented
          * @param delta the amount to increment the current value by to reach the goal
          *
          * @return the current value + the delta, or the goal if the goal is met
          */
        int approach(int goal, int current, int delta);
};

#endif