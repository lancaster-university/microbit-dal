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

#ifndef MICROBIT_COMPASS_CALIBRATOR_H
#define MICROBIT_COMPASS_CALIBRATOR_H

#include "MicroBitConfig.h"
#include "MicroBitCompass.h"
#ifdef TARGET_NRF51_CALLIOPE
#include "MicroBitAccelerometer-bmx.h"
#else
#include "MicroBitAccelerometer.h"
#endif
#include "MicroBitDisplay.h"


/**
  * Class definition for an interactive compass calibration algorithm.
  *
  * The algorithm uses an accelerometer to ensure that a broad range of sample data has been gathered
  * from the compass module, then performs a least mean squares optimisation of the
  * results to determine the calibration data for the compass.
  *
  * The LED matrix display is used to provide feedback to the user on the gestures required.
  *
  * This class listens for calibration requests from the compass (on the default event model),
  * and automatically initiates a calibration sequence as necessary.
  */
class MicroBitCompassCalibrator
{
    MicroBitCompass&        compass;
    MicroBitAccelerometer&  accelerometer;
    MicroBitDisplay&        display;

    public:

    /**
      * Constructor.
      *
      * Create an object capable of calibrating the compass.
      *
      * The algorithm uses an accelerometer to ensure that a broad range of sample data has been gathered
      * from the compass module, then performs a least mean squares optimisation of the
      * results to determine the calibration data for the compass.
      *
      * The LED matrix display is used to provide feedback to the user on the gestures required.
      *
      * @param compass The compass instance to calibrate.
      *
      * @param accelerometer The accelerometer to gather contextual data from.
      *
      * @param display The LED matrix to display user feedback on.
      */
    MicroBitCompassCalibrator(MicroBitCompass& _compass, MicroBitAccelerometer& _accelerometer, MicroBitDisplay& _display);

    /**
      * Performs a simple game that in parallel, calibrates the compass.
      *
      * This function is executed automatically when the user requests a compass bearing, and compass calibration is required.
      *
      * This function is, by design, synchronous and only returns once calibration is complete.
      */
    void calibrate(MicroBitEvent);
};

#endif
