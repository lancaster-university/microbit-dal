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
#include "MicroBitSystemTimer.h"
#include "MicroBitEnergyMonitor.h"

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
    this->minFieldStrength = 2147483647; // set minFieldStrength value to "infinity"
    this->maxFieldStrength = -2147483646; // set maxFieldStrength value to "-infinity"

    fiber_add_idle_component(this);
}

/**
  * Periodic callback from MicroBit idle thread.
  */
void MicroBitEnergyMonitor::idleTick()
{
    updateSamples();
    updateEvents();
}

/**
  * Processes one sample from the magnetometer and calculates the energy usage (watts) when a set
  * amount of samples have been processed.
  *
  * @return the current sample count
  */
int MicroBitEnergyMonitor::updateSamples()
{    
    int fieldStrength = magnetometer.getZ();
    
    // update sample min and max
    minFieldStrength = min(minFieldStrength, fieldStrength);
    maxFieldStrength = max(maxFieldStrength, fieldStrength);
    
    sample++;
    
    // if not enough samples have been processed, leave
    if(sample < SAMPLES)
        return sample;
    
    // when enough sampels have been gathered, calculate the amplitude and watts
    amplitude = maxFieldStrength - minFieldStrength; // get the amplitude of the current values
    
    // map the amplitude to watts
    watts = map(amplitude, RANGE_MIN, RANGE_MAX, 0, WATTAGE_MAX); // updates usage
    
    sample = 0; // reset sasmple counter
    minFieldStrength = 2147483647; // reset minFieldStrength value to "infinity"
    maxFieldStrength = -2147483646; // reset maxFieldStrength value to "-infinity"
    
    return sample;
}

/**
  * Checks for state changes of the electrical power and fires various events on a state change.
  */
int MicroBitEnergyMonitor::updateEvents()
{
    if(isCalibrating())
        return MICROBIT_ENERGY_MONITOR_CALIBRATING;
    
    // check to see if we have off->on state change
    if(isElectricalPowerOn() && !(status & MICROBIT_ENERGY_MONITOR_STATE))
    {
        // record we have a state change, and raise an event
        status |= MICROBIT_ENERGY_MONITOR_STATE;
        MicroBitEvent evt(this->id, MICROBIT_ENERGY_MONITOR_EVT_ON);
    }
    
    // check to see if we have on->off state change
    if(!isElectricalPowerOn() && (status & MICROBIT_ENERGY_MONITOR_STATE))
    {
        // record state change, and raise an event
        status = 0;
        MicroBitEvent evt(this->id, MICROBIT_ENERGY_MONITOR_EVT_OFF);
    }
    
    return MICROBIT_OK;
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
  * Used for for sampling the amplitude of the magnetometer samples.
  *
  * @returns the amplitude of the current sample set
  */
int MicroBitEnergyMonitor::getAmplitude()
{
    return amplitude;
}

/**
  * Assists in calibrating the position of the microbit to best sense electrical power.
  *
  * @code
  * monitor.calibrate();
  * @endcode
  */
void MicroBitEnergyMonitor::calibrate()
{
    // record that we've started calibrating
    status |= MICROBIT_ENERGY_MONITOR_CALIBRATING;

    // launch any registred calibration alogrithm
    MicroBitEvent evt(this->id, MICROBIT_ENERGY_MONITOR_EVT_CALIBRATE);
}

/**
  * Returns whether or not the energy monitor is being calibrated.
  * 
  * @code
  * if(monitor.isCalibrating())
  *   serial.send("Energy Monitor is calibrating!");
  * @endcode
  */
bool MicroBitEnergyMonitor::isCalibrating()
{
    return (status & MICROBIT_ENERGY_MONITOR_CALIBRATING);
}

/**
  * Removes the calibration status flag.
  * 
  * @code
  * monitor.stopCalibration();
  * @endcode
  */
void MicroBitEnergyMonitor::stopCalibration()
{
    // record that we've finished calibrating
    status &= ~MICROBIT_ENERGY_MONITOR_CALIBRATING;
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