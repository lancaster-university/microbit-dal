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
 * Scoring function for a hill climb algorithm.
 *
 * @param c An approximated centre point
 * @param data a collection of data points
 * @param samples the number of samples in the 'data' array
 *
 * @return The deviation between the closest and further point in the data array from the point given. 
 */
int MicroBitCompassCalibrator::measureScore(CompassSample &c, CompassSample *data, int samples)
{
    int minD;
    int maxD;

    minD = maxD = c.dSquared(data[0]);
    for (int i = 1; i < samples; i++)
    {
        int d = c.dSquared(data[i]);

        if (d < minD)
            minD = d;

        if (d > maxD)
            maxD = d;
    }

    return (maxD - minD);
}

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
CompassCalibration MicroBitCompassCalibrator::spherify(CompassSample centre, CompassSample *data, int samples)
{
    // First, determine the radius of the enclosing sphere from the given centre.
    // n.b. this will likely be different to the radius from the centre of mass previously calculated.
    // We use the same algorithm though.
    CompassCalibration result;

    float radius = 0;
    float scaleX = 0.0;
    float scaleY = 0.0;
    float scaleZ = 0.0;

    float scale = 0.0;
    float weightX = 0.0;
    float weightY = 0.0;
    float weightZ = 0.0;

    for (int i = 0; i < samples; i++)
    {
        int d = sqrt((float)centre.dSquared(data[i]));

        if (d > radius)
            radius = d;
    }

    // Now, for each data point, determine a scalar multiplier for the vector between the centre and that point that
    // takes the point onto the surface of the enclosing sphere.
    for (int i = 0; i < samples; i++)
    {
        // Calculate the distance from this point to the centre of the sphere
        float d = sqrt(centre.dSquared(data[i]));

        // Now determine a scalar multiplier that, when applied to the vector to the centre,
        // will place this point on the surface of the sphere.
        float s = (radius / d) - 1;

        scale = max(scale, s);

        // next, determine the scale effect this has on each of our components.
        float dx = (data[i].x - centre.x); 
        float dy = (data[i].y - centre.y);
        float dz = (data[i].z - centre.z);

        weightX += s * fabsf(dx / d);
        weightY += s * fabsf(dy / d);
        weightZ += s * fabsf(dz / d);
    }

    float wmag = sqrt((weightX * weightX) + (weightY * weightY) + (weightZ * weightZ));

    scaleX = 1.0 + scale * (weightX / wmag);
    scaleY = 1.0 + scale * (weightY / wmag);
    scaleZ = 1.0 + scale * (weightZ / wmag);

    result.scale.x = (int)(1024 * scaleX);
    result.scale.y = (int)(1024 * scaleY);
    result.scale.z = (int)(1024 * scaleZ);

    result.centre.x = centre.x;
    result.centre.y = centre.y;
    result.centre.z = centre.z;

    result.radius = radius;

    return result;
}

/*
 * Performs an interative approximation (hill descent) algorithm to determine an
 * estimated centre point of a sphere upon which the given data points reside.
 *
 * @param data an array containing sample points
 * @param samples the number of sample points in the 'data' array.
 *
 * @return the approximated centre point of the points in the 'data' array.
 */
CompassSample MicroBitCompassCalibrator::approximateCentre(CompassSample *data, int samples)
{
    CompassSample c,t;
    CompassSample centre = { 0,0,0 };
    CompassSample best = { 0,0,0 };

    int score;

    for (int i = 0; i < samples; i++)
    {
        centre.x += data[i].x;
        centre.y += data[i].y;
        centre.z += data[i].z;
    }

    // Calclulate a centre of mass for our input samples. We only use this for validation purposes.
    centre.x = centre.x / samples;
    centre.y = centre.y / samples;
    centre.z = centre.z / samples;

    // Start hill climb in the centre of mass.
    c = centre;

    // calculate the nearest and furthest point to us.
    score = measureScore(c, data, samples);

    // iteratively attempt to improve position...
    while (1)
    {
        for (int x = -1; x <= 1; x++)
        {
            for (int y = -1; y <= 1; y++)
            {
                for (int z = -1; z <= 1; z++)
                {
                    t = c;
                    t.x += x;
                    t.y += y;
                    t.z += z;

                    int s = measureScore(t, data, samples);
                    if (s < score)
                    {
                        score = s;
                        best = t;
                    }
                }
            }
        }

        if (best.x == c.x && best.y == c.y && best.z == c.z)
            break;

        c = best;
    }

    return c;
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

    const int PERIMETER_POINTS = 21;
    //const int PERIMETER_POINTS = 12;

    const int PIXEL1_THRESHOLD = 200;
    const int PIXEL2_THRESHOLD = 800;

    wait_ms(100);

    Point perimeter[PERIMETER_POINTS] = {{1,0,0}, {2,0,0}, {3,0,0}, {0,1,0}, {1,1,0}, {2,1,0}, {3,1,0}, {4,1,0}, {0,2,0}, {1,2,0}, {2,2,0}, {3,2,0}, {4,2,0}, {0,3,0}, {1,3,0}, {2,3,0}, {3,3,0}, {4,3,0}, {1,4,0}, {2,4,0}, {3,4,0}};
    //Point perimeter[PERIMETER_POINTS] = {{1,0,0}, {2,0,0}, {3,0,0}, {4,1,0}, {4,2,0}, {4,3,0}, {3,4,0}, {2,4,0}, {1,4,0}, {0,3,0}, {0,2,0}, {0,1,0}};
    Point cursor = {2,2,0};

    MicroBitImage img(5,5);
    MicroBitImage smiley("0,255,0,255,0\n0,255,0,255,0\n0,0,0,0,0\n255,0,0,0,255\n0,255,255,255,0\n");

    CompassSample data[PERIMETER_POINTS];
    int samples = 0;

    // Firstly, we need to take over the display. Ensure all active animations are paused.
    display.stopAnimation();
    display.scrollAsync("TILT A CIRCLE");

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
                data[samples].x = compass.getX(RAW);
                data[samples].y = compass.getY(RAW);
                data[samples].z = compass.getZ(RAW);

                // Record that this pixel has been visited.
                perimeter[i].on = 1;
                samples++;
            }
        }

        wait_ms(100);
    }

    CompassSample centre = approximateCentre(data, samples);
    CompassCalibration cal = spherify(centre, data, samples);
    compass.setCalibration(cal);

    // Show a smiley to indicate that we're done, and continue on with the user program.
    display.clear();
    display.printAsync(smiley, 0, 0, 0, 1500);
    wait_ms(1000);
    display.clear();
}




#ifdef OLD_CALIBRATION
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

#endif
