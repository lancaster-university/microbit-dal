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

#include "MicroBitConfig.h"
#include "MicroBitEnergyMonitor.h"
#include "MicroBitSystemTimer.h"

/**
  * Constructor that initialises the micro:bit magnetometer, the default status, and initialises the idleTick.
  *
  * @param magnetometer an instance of the micro:bit's magnetometer component
  *
  * @code
  * MicroBitEnergyMonitor monitor;
  * monitor.getEnergyUsage();
  * @endcode
  */
MicroBitEnergyMonitor::MicroBitEnergyMonitor(MicroBitCompass &magnetometer, uint16_t id) : magnetometer(magnetometer)
{
    this->magnetometer.setPeriod(1);
    this->id = id;
    this->status = 0x00;
    this->sample = 0;
    this->watts = 0;
    this->amplitude = 0;

    fiber_add_idle_component(this);
}

int MicroBitEnergyMonitor::updateSamples()
{
    // if calibration is in progress, leave
    if(magnetometer.isCalibrating())
        return MICROBIT_CALIBRATION_IN_PROGRESS;
    
    int fieldStrength = magnetometer.getZ();
    
    // update sample min and max
    maxFieldStrength = max(maxFieldStrength, fieldStrength);
    minFieldStrength = min(minFieldStrength, fieldStrength);
    
    sample++;
    
    // if not enough samples have been processed, leave
    if(sample < SAMPLES)
        return MICROBIT_OK;
    
    // when enough sampels have been gathered, calculate the amplitude and watts
    amplitude = maxFieldStrength - minFieldStrength; // get the amplitude of the current values
    
    // map the amplitude to watts
    watts = map(amplitude, RANGE_MIN, RANGE_MAX, 0, WATTAGE_MAX); // updates usage
    
    sample = 0; // reset sasmple counter
    minFieldStrength = 2147483647; // reset minFieldStrength value to "infinity"
    maxFieldStrength = -2147483646; // reset maxFieldStrength value to "-infinity"
    
    // check to see if we have off->on state change
    if(isElectricalPowerOn() && !(status & MICROBIT_ELECTRICAL_POWER_STATE))
    {
        // record we have a state change, and raise an event
        status |= MICROBIT_ELECTRICAL_POWER_STATE;
        MicroBitEvent evt(this->id, MICROBIT_ELECTRICAL_POWER_EVT_ON);
    }
    
    // check to see if we have on->off state change
    if(!isElectricalPowerOn() && (status & MICROBIT_ELECTRICAL_POWER_STATE))
    {
        // record state change, and raise an event
        status = 0;
        MicroBitEvent evt(this->id, MICROBIT_ELECTRICAL_POWER_EVT_OFF);
    }
    
    return MICROBIT_OK;
}

/**
  * Periodic callback from MicroBit idle thread.
  */
void MicroBitEnergyMonitor::idleTick()
{
    updateSamples();
}

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
int MicroBitEnergyMonitor::isElectricalPowerOn()
{
    return getEnergyUsage() > 0 ? 1 : 0;
}

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
int MicroBitEnergyMonitor::getEnergyUsage()
{
    return watts;
}


/**
  * Calibrates the magnetometer by calling the standard calibration function from the MicroBitCompass component.
  *
  * NOTE: Ensure micro:bit is not near an in-use cable when calibrating.
  */
void MicroBitEnergyMonitor::calibrate()
{
    magnetometer.calibrate();
}

/**
  * Used for debug purposes for sampling the amplitude of the magnetometer samples.
  *
  * @returns the amplitude of the current sample set
  */
int MicroBitEnergyMonitor::getAmplitude()
{
    return amplitude;
}

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
int MicroBitEnergyMonitor::map(int value, int fromLow, int fromHigh, int toLow, int toHigh)
{
    int val = ((value - fromLow) * (toHigh - toLow)) / (fromHigh - fromLow) + toLow;
    if(val < 0)
        return 0;
    return val;
}

/**
  * Destructor for MicroBitButton, where we deregister this instance from the array of fiber components.
  */
MicroBitEnergyMonitor::~MicroBitEnergyMonitor()
{
    fiber_remove_idle_component(this);
}