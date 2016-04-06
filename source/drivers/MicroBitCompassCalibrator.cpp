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
#include "MicroBitCompassCalibrator.h"
#include "EventModel.h"
#include "Matrix4.h"

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
MicroBitCompassCalibrator::MicroBitCompassCalibrator(MicroBitCompass& _compass, MicroBitAccelerometer& _accelerometer, MicroBitDisplay& _display) : compass(_compass), accelerometer(_accelerometer), display(_display)
{
    if (EventModel::defaultEventBus)
        EventModel::defaultEventBus->listen(MICROBIT_ID_COMPASS, MICROBIT_COMPASS_EVT_CALIBRATE, this, &MicroBitCompassCalibrator::calibrate, MESSAGE_BUS_LISTENER_IMMEDIATE);
}

/**
  * Performs a simple game that in parallel, calibrates the compass.
  *
  * This function is executed automatically when the user requests a compass bearing, and compass calibration is required.
  *
  * This function is, by design, synchronous and only returns once calibration is complete.
  */
void MicroBitCompassCalibrator::calibrate(MicroBitEvent)
{
    struct Point
    {
        uint8_t x;
        uint8_t y;
        uint8_t on;
    };

    const int PERIMETER_POINTS = 12;
    const int PIXEL1_THRESHOLD = 200;
    const int PIXEL2_THRESHOLD = 800;

    wait_ms(100);

	Matrix4 X(PERIMETER_POINTS, 4);
    Point perimeter[PERIMETER_POINTS] = {{1,0,0}, {2,0,0}, {3,0,0}, {4,1,0}, {4,2,0}, {4,3,0}, {3,4,0}, {2,4,0}, {1,4,0}, {0,3,0}, {0,2,0}, {0,1,0}};
    Point cursor = {2,2,0};

    MicroBitImage img(5,5);
    MicroBitImage smiley("0,255,0,255,0\n0,255,0,255,0\n0,0,0,0,0\n255,0,0,0,255\n0,255,255,255,0\n");
    int samples = 0;

    // Firstly, we need to take over the display. Ensure all active animations are paused.
    display.stopAnimation();
    display.scrollAsync("DRAW A CIRCLE");

    for (int i=0; i<110; i++)
        wait_ms(100);

    display.stopAnimation();
    display.clear();

    while(samples < PERIMETER_POINTS)
    {
        // update our model of the flash status of the user controlled pixel.
        cursor.on = (cursor.on + 1) % 4;

        // take a snapshot of the current accelerometer data.
        int x = accelerometer.getX();
        int y = accelerometer.getY();

        // Wait a little whie for the button state to stabilise (one scheduler tick).
        wait_ms(10);

        // Deterine the position of the user controlled pixel on the screen.
        if (x < -PIXEL2_THRESHOLD)
            cursor.x = 0;
        else if (x < -PIXEL1_THRESHOLD)
            cursor.x = 1;
        else if (x > PIXEL2_THRESHOLD)
            cursor.x = 4;
        else if (x > PIXEL1_THRESHOLD)
            cursor.x = 3;
        else
            cursor.x = 2;

        if (y < -PIXEL2_THRESHOLD)
            cursor.y = 0;
        else if (y < -PIXEL1_THRESHOLD)
            cursor.y = 1;
        else if (y > PIXEL2_THRESHOLD)
            cursor.y = 4;
        else if (y > PIXEL1_THRESHOLD)
            cursor.y = 3;
        else
            cursor.y = 2;

        img.clear();

        // Turn on any pixels that have been visited.
        for (int i=0; i<PERIMETER_POINTS; i++)
            if (perimeter[i].on)
                img.setPixelValue(perimeter[i].x, perimeter[i].y, 255);

        // Update the pixel at the users position.
        img.setPixelValue(cursor.x, cursor.y, 255);

        // Update the buffer to the screen.
        display.image.paste(img,0,0,0);

        // test if we need to update the state at the users position.
        for (int i=0; i<PERIMETER_POINTS; i++)
        {
            if (cursor.x == perimeter[i].x && cursor.y == perimeter[i].y && !perimeter[i].on)
            {
                // Record the sample data for later processing...
                X.set(samples, 0, compass.getX(RAW));
                X.set(samples, 1, compass.getY(RAW));
                X.set(samples, 2, compass.getZ(RAW));
                X.set(samples, 3, 1);

                // Record that this pixel has been visited.
                perimeter[i].on = 1;
                samples++;
            }
        }

        wait_ms(100);
    }

    // We have enough sample data to make a fairly accurate calibration.
    // We use a Least Mean Squares approximation, as detailed in Freescale application note AN2426.

    // Firstly, calculate the square of each sample.
	Matrix4 Y(X.height(), 1);
	for (int i = 0; i < X.height(); i++)
	{
		float v = X.get(i, 0)*X.get(i, 0) + X.get(i, 1)*X.get(i, 1) + X.get(i, 2)*X.get(i, 2);
		Y.set(i, 0, v);
	}

    // Now perform a Least Squares Approximation.
	Matrix4 Alpha = X.multiplyT(X).invert();
	Matrix4 Gamma = X.multiplyT(Y);
	Matrix4 Beta = Alpha.multiply(Gamma);

    // The result contains the approximate zero point of each axis, but doubled.
    // Halve each sample, and record this as the compass calibration data.
    CompassSample cal ((int)(Beta.get(0,0) / 2), (int)(Beta.get(1,0) / 2), (int)(Beta.get(2,0) / 2));
    compass.setCalibration(cal);

    // Show a smiley to indicate that we're done, and continue on with the user program.
    display.clear();
    display.printAsync(smiley, 0, 0, 0, 1500);
    wait_ms(1000);
    display.clear();
}
