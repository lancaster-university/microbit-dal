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

#include "EventModel.h"
#include "MicroBitConfig.h"
#include "MicroBitEnergyMonitor.h"
#include "MicroBitEnergyMonitorCalibrator.h"

/**
  * Constructor.
  *
  * Create an object capable of calibrating the microbit's position for energy monitoring.
  *
  * The algorithm uses the strength of the electromagnetic field around the cable and displays
  * the current relative strength at a given position.
  *
  * The LED matrix display is used to provide feedback to the user on the strength of the electromagnetic
  * field strength in the form of a line from levels 1 (weakest) to 5 (strongest).
  *
  * @param monitor The energy monitor driver
  * @param display The LED matrix to display user feedback on
  */
MicroBitEnergyMonitorCalibrator::MicroBitEnergyMonitorCalibrator(MicroBitEnergyMonitor& _monitor, MicroBitDisplay& _display) : monitor(_monitor), display(_display)
{
    if (EventModel::defaultEventBus)
    {
        EventModel::defaultEventBus->listen(MICROBIT_ID_ENERGY_MONITOR, MICROBIT_ENERGY_MONITOR_EVT_CALIBRATE, this, &MicroBitEnergyMonitorCalibrator::calibrateUX);
    }
}

/**
  * Performs a simple game that in parallel, calibrates the position of the microbit for more
  * accurate energy monitoring.
  */
void MicroBitEnergyMonitorCalibrator::calibrateUX(MicroBitEvent)
{
    // listen on A + B click event to end the calibration sequence
    if (EventModel::defaultEventBus)
        EventModel::defaultEventBus->listen(MICROBIT_ID_BUTTON_AB, MICROBIT_BUTTON_EVT_CLICK, this, &MicroBitEnergyMonitorCalibrator::stopCalibration, MESSAGE_BUS_LISTENER_IMMEDIATE);
        
    MicroBitImage smiley("0,255,0,255,0\n0,255,0,255,0\n0,0,0,0,0\n255,0,0,0,255\n0,255,255,255,0\n");
    
    int displayBrightness = display.getBrightness();
    
    int minAmplitude = 2147483647;
    int maxAmplitude = -2147483646;
    int amplitude = monitor.getAmplitude();
    int strength = 0;
    int lastStrength = -1;
    
    wait_ms(1000);
    
    // firstly, we need to take over the display. Ensure all active animations are paused.
    display.stopAnimation();
    display.setBrightness(255); // max brightness
    
    display.scroll("TURN SLOWLY"); // basic instructions to not hold up the display

    while(monitor.isCalibrating())
    {
        while(monitor.updateSamples() != 0); // force update the samples in the monitor driver (take over idleTick)
        
        amplitude = monitor.getAmplitude();        
        minAmplitude = min(minAmplitude, amplitude);
        maxAmplitude = max(maxAmplitude, amplitude);
        strength = monitor.map(amplitude, minAmplitude, maxAmplitude, 0, 4); // update the strength to 0-4
        
        // only update display if the strength has changed (prevents lots of display updating)
        if(lastStrength != strength)
        {
            display.clear();
            
            for(int x = 0; x < 5; x++)
                display.image.setPixelValue(x, 4 - strength, 255); // draw line accross display at current strength
        }
        
        lastStrength = strength;
    }

    // display a smiley face to indicate the end of the calibration process
    display.clear();
    display.printAsync(smiley, 0, 0, 0, 1500);
    wait_ms(1000);
    display.clear();

    // retore the display brightness to the level it was at before this function was called.
    display.setBrightness(displayBrightness);
    
    // remove A + B event callback
    if (EventModel::defaultEventBus)
        EventModel::defaultEventBus->ignore(MICROBIT_ID_BUTTON_AB, MICROBIT_BUTTON_EVT_CLICK, this, &MicroBitEnergyMonitorCalibrator::stopCalibration);
}

/**
  * Calls the stopCalibration() method from the energy monitor driver to remove the calibration status flag.
  */
void MicroBitEnergyMonitorCalibrator::stopCalibration(MicroBitEvent)
{
    monitor.stopCalibration();
}