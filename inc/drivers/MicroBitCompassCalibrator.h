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
#include "MicroBitAccelerometer.h"
#include "MicroBitDisplay.h"
#include "MicroBitStorage.h"


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
    MicroBitStorage         *storage;

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
      * @param accelerometer The accelerometer to gather contextual data from.
      * @param display The LED matrix to display user feedback on.
      * @param storage The object to use for storing calibration data in persistent FLASH. 
      */
    MicroBitCompassCalibrator(MicroBitCompass& _compass, MicroBitAccelerometer& _accelerometer, MicroBitDisplay& _display, MicroBitStorage &storage);

    /**
      * Performs a simple game that in parallel, calibrates the compass.
      *
      * This function is executed automatically when the user requests a compass bearing, and compass calibration is required.
      *
      * This function is, by design, synchronous and only returns once calibration is complete.
      */
    void calibrateUX(MicroBitEvent);
     /**
      * Calculates an independent X, Y, Z scale factor and centre for a given set of data points,
      * assumed to be on a bounding sphere
      *
      * @param data An array of all data points
      * @param samples The number of samples in the 'data' array.
      *
      * This algorithm should be called with no fewer than 12 points, but testing has indicated >21
      * points provides a more robust calculation.
      *
      * @return A calibration structure containing the a calculated centre point, the radius of the
      * minimum enclosing spere of points and a scaling factor for each axis that places those
      * points as close as possible to the surface of the containing sphere.
      */
     static CompassCalibration calibrate(Sample3D *data, int samples);

    private:

    /**
     * Scoring function for a hill climb algorithm.
     *
     * @param c An approximated centre point
     * @param data a collection of data points
     * @param samples the number of samples in the 'data' array
     *
     * @return The deviation between the closest and further point in the data array from the point given. 
     */
    static float measureScore(Sample3D &c, Sample3D *data, int samples);

    /*
     * Performs an interative approximation (hill descent) algorithm to determine an
     * estimated centre point of a sphere upon which the given data points reside.
     *
     * @param data an array containing sample points
     * @param samples the number of sample points in the 'data' array.
     *
     * @return the approximated centre point of the points in the 'data' array.
     */
    static Sample3D approximateCentre(Sample3D *data, int samples);

    /**
     * Calculates an independent scale factor for X,Y and Z axes that places the given data points on a bounding sphere
     *
     * @param centre A proviously calculated centre point of all data.
     * @param data An array of all data points
     * @param samples The number of samples in the 'data' array.
     *
     * @return A calibration structure containing the centre point provided, the radius of the minimum
     * enclosing spere of points and a scaling factor for each axis that places those points as close as possible
     * to the surface of the containing sphere.
     */
    static CompassCalibration spherify(Sample3D centre, Sample3D *data, int samples);
};

#endif
