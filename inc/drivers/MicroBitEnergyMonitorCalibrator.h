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

#ifndef MICROBIT_ENERGY_MONITOR_CALIBRATOR_H
#define MICROBIT_ENERGY_MONITOR_CALIBRATOR_H

#include "MicroBitConfig.h"
#include "MicroBitButton.h"
#include "MicroBitDisplay.h"
#include "MicroBitEnergyMonitor.h"

/**
  * Class definition for an interactive compass calibration algorithm.
  *
  * The algorithm uses an accelerometer to ensure that a broad range of sample data has been gathered
  * from the magnetometer around the cable.
  *
  * The LED matrix display is used to provide feedback to the user on the gestures required.
  *
  * This class listens for calibration requests from the compass (on the default event model),
  * and automatically initiates a calibration sequence as necessary.
  */
class MicroBitEnergyMonitorCalibrator
{
    protected:
    
        MicroBitEnergyMonitor&  monitor;
        MicroBitDisplay&        display;

    public:
	
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
		MicroBitEnergyMonitorCalibrator(MicroBitEnergyMonitor& _monitor, MicroBitDisplay& _display);
		
		 /**
		  * Performs a simple game that in parallel, calibrates the position of the microbit for more
          * accurate energy monitoring.
		  */
		void calibrateUX(MicroBitEvent);
        
    private:
    
        /**
          * Calls the stopCalibration() method from the energy monitor driver to remove the calibration status flag.
          */
        void stopCalibration(MicroBitEvent);
};

#endif