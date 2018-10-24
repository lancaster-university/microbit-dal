/*
The MIT License (MIT)

Copyright (c) 2017 Lancaster University.

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

#include "MicroBitAccelerometer.h"
#include "ErrorNo.h"
#include "MicroBitEvent.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"
#include "MicroBitDevice.h"

#include "MicroBitI2C.h"
#include "MMA8653.h"
#include "FXOS8700.h"
#include "LSM303Accelerometer.h"

/**
  * Constructor.
  * Create a software abstraction of an FXSO8700 combined accelerometer/magnetometer
  *
  * @param _i2c an instance of I2C used to communicate with the device.
  *
  * @param address the default I2C address of the accelerometer. Defaults to: FXS8700_DEFAULT_ADDR.
  *
 */
MicroBitAccelerometer::MicroBitAccelerometer(CoordinateSpace &cspace, uint16_t id) : sample(), sampleENU(), coordinateSpace(cspace)
{
    // Store our identifiers.
    this->id = id;
    this->status = 0;

    // Set a default rate of 50Hz and a +/-2g range.
    this->samplePeriod = 20;
    this->sampleRange = 2;

    // Initialise gesture history
    this->sigma = 0;
    this->impulseSigma = 0;
    this->lastGesture = MICROBIT_ACCELEROMETER_EVT_NONE;
    this->currentGesture = MICROBIT_ACCELEROMETER_EVT_NONE;
    this->shake.x = 0;
    this->shake.y = 0;
    this->shake.z = 0;
    this->shake.count = 0;
    this->shake.timer = 0;
    this->shake.impulse_3 = 1;
    this->shake.impulse_6 = 1;
    this->shake.impulse_8 = 1;
}

/**
 * Device autodetection. Scans the given I2C bus for supported accelerometer devices.
 * if found, constructs an appropriate driver and returns it.
 *
 * @param i2c the bus to scan. 
 * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_ACCELEROMETER
 *
 */
MicroBitAccelerometer& MicroBitAccelerometer::autoDetect(MicroBitI2C &i2c)
{
    if (MicroBitAccelerometer::detectedAccelerometer == NULL)
    {
        // Configuration of IRQ lines
        MicroBitPin int1(MICROBIT_ID_IO_INT1, P0_28, PIN_CAPABILITY_STANDARD);
        MicroBitPin int2(MICROBIT_ID_IO_INT2, P0_29, PIN_CAPABILITY_STANDARD);
        MicroBitPin int3(MICROBIT_ID_IO_INT3, P0_27, PIN_CAPABILITY_STANDARD);

        // All known accelerometer/magnetometer peripherals have the same alignment
        CoordinateSpace &coordinateSpace = *(new CoordinateSpace(SIMPLE_CARTESIAN, true, COORDINATE_SPACE_ROTATED_0));

        // Now, probe for connected peripherals, if none have already been found.
        if (MMA8653::isDetected(i2c))
            MicroBitAccelerometer::detectedAccelerometer = new MMA8653(i2c, int1, coordinateSpace);

        else if (LSM303Accelerometer::isDetected(i2c))
            MicroBitAccelerometer::detectedAccelerometer = new LSM303Accelerometer(i2c, int1, coordinateSpace);

        else if (FXOS8700::isDetected(i2c))
        {
            FXOS8700 *fxos =  new FXOS8700(i2c, int3, coordinateSpace);
            MicroBitAccelerometer::detectedAccelerometer = fxos;
            MicroBitCompass::detectedCompass = fxos;
        }

        // Insert this case to support FXOS on the microbit1.5-SN
        //else if (FXOS8700::isDetected(i2c, 0x3A))
        //{
        //    FXOS8700 *fxos =  new FXOS8700(i2c, int3, coordinateSpace, 0x3A);
        //    MicroBitAccelerometer::detectedAccelerometer = fxos;
        //    MicroBitCompass::detectedCompass = fxos;
        //}

        else
        {
            MicroBitAccelerometer *unavailable =  new MicroBitAccelerometer(coordinateSpace, MICROBIT_ID_ACCELEROMETER);
            MicroBitAccelerometer::detectedAccelerometer = unavailable;
        }
    }

    if (MicroBitCompass::detectedCompass)
        MicroBitCompass::detectedCompass->setAccelerometer(*MicroBitAccelerometer::detectedAccelerometer);

    return *MicroBitAccelerometer::detectedAccelerometer;
}


/**
  * Stores data from the accelerometer sensor in our buffer, and perform gesture tracking.
  *
  * On first use, this member function will attempt to add this component to the
  * list of fiber components in order to constantly update the values stored
  * by this object.
  *
  * This lazy instantiation means that we do not
  * obtain the overhead from non-chalantly adding this component to fiber components.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the read request fails.
  */
int MicroBitAccelerometer::update()
{
    // Store the new data, after performing any necessary coordinate transformations.
    sample = coordinateSpace.transform(sampleENU);

    // Indicate that pitch and roll data is now stale, and needs to be recalculated if needed.
    status &= ~MICROBIT_ACCELEROMETER_IMU_DATA_VALID;

    // Update gesture tracking
    updateGesture();

    // Indicate that a new sample is available
    MicroBitEvent e(id, MICROBIT_ACCELEROMETER_EVT_DATA_UPDATE);

    return MICROBIT_OK;
};

/**
  * A service function.
  * It calculates the current scalar acceleration of the device (x^2 + y^2 + z^2).
  * It does not, however, square root the result, as this is a relatively high cost operation.
  *
  * This is left to application code should it be needed.
  *
  * @return the sum of the square of the acceleration of the device across all axes.
  */
uint32_t MicroBitAccelerometer::instantaneousAccelerationSquared()
{
    // Use pythagoras theorem to determine the combined force acting on the device.
    return (uint32_t)sample.x*(uint32_t)sample.x + (uint32_t)sample.y*(uint32_t)sample.y + (uint32_t)sample.z*(uint32_t)sample.z;
}

/**
 * Service function.
 * Determines a 'best guess' posture of the device based on instantaneous data.
 *
 * This makes no use of historic data, and forms the input to the filter implemented in updateGesture().
 *
 * @return A 'best guess' of the current posture of the device, based on instanataneous data.
 */
uint16_t MicroBitAccelerometer::instantaneousPosture()
{
    bool shakeDetected = false;

    // Test for shake events.
    // We detect a shake by measuring zero crossings in each axis. In other words, if we see a strong acceleration to the left followed by
    // a strong acceleration to the right, then we can infer a shake. Similarly, we can do this for each axis (left/right, up/down, in/out).
    //
    // If we see enough zero crossings in succession (MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD), then we decide that the device
    // has been shaken.
    if ((sample.x < -MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && shake.x) || (sample.x > MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && !shake.x))
    {
        shakeDetected = true;
        shake.x = !shake.x;
    }

    if ((sample.y < -MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && shake.y) || (sample.y > MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && !shake.y))
    {
        shakeDetected = true;
        shake.y = !shake.y;
    }

    if ((sample.z < -MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && shake.z) || (sample.z > MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && !shake.z))
    {
        shakeDetected = true;
        shake.z = !shake.z;
    }

    // If we detected a zero crossing in this sample period, count this.
    if (shakeDetected && shake.count < MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD)
    {
        shake.count++;

        if (shake.count == 1)
            shake.timer = 0;

        if (shake.count == MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD)
        {
            shake.shaken = 1;
            shake.timer = 0;
            return MICROBIT_ACCELEROMETER_EVT_SHAKE;
        }
    }

    // measure how long we have been detecting a SHAKE event.
    if (shake.count > 0)
    {
        shake.timer++;

        // If we've issued a SHAKE event already, and sufficient time has assed, allow another SHAKE event to be issued.
        if (shake.shaken && shake.timer >= MICROBIT_ACCELEROMETER_SHAKE_RTX)
        {
            shake.shaken = 0;
            shake.timer = 0;
            shake.count = 0;
        }

        // Decay our count of zero crossings over time. We don't want them to accumulate if the user performs slow moving motions.
        else if (!shake.shaken && shake.timer >= MICROBIT_ACCELEROMETER_SHAKE_DAMPING)
        {
            shake.timer = 0;
            if (shake.count > 0)
                shake.count--;
        }
    }

    uint32_t force = instantaneousAccelerationSquared();
    if (force < MICROBIT_ACCELEROMETER_FREEFALL_THRESHOLD)
        return MICROBIT_ACCELEROMETER_EVT_FREEFALL;

    // Determine our posture.
    if (sample.x < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_LEFT;

    if (sample.x > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_RIGHT;

    if (sample.y < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_DOWN;

    if (sample.y > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_UP;

    if (sample.z < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_FACE_UP;

    if (sample.z > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_FACE_DOWN;

    return MICROBIT_ACCELEROMETER_EVT_NONE;
}

/**
  * Updates the basic gesture recognizer. This performs instantaneous pose recognition, and also some low pass filtering to promote
  * stability.
  */
void MicroBitAccelerometer::updateGesture()
{
    // Check for High/Low G force events - typically impulses, impacts etc.
    // Again, during such spikes, these event take priority of the posture of the device.
    // For these events, we don't perform any low pass filtering.
    uint32_t force = instantaneousAccelerationSquared();

    if (force > MICROBIT_ACCELEROMETER_3G_THRESHOLD)
    {
        if (force > MICROBIT_ACCELEROMETER_3G_THRESHOLD && !shake.impulse_3)
        {
            MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_3G);
            shake.impulse_3 = 1;
        }
        if (force > MICROBIT_ACCELEROMETER_6G_THRESHOLD && !shake.impulse_6)
        {
            MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_6G);
            shake.impulse_6 = 1;
        }
        if (force > MICROBIT_ACCELEROMETER_8G_THRESHOLD && !shake.impulse_8)
        {
            MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_8G);
            shake.impulse_8 = 1;
        }

        impulseSigma = 0;
    }

    // Reset the impulse event onve the acceleration has subsided.
    if (impulseSigma < MICROBIT_ACCELEROMETER_GESTURE_DAMPING)
        impulseSigma++;
    else
        shake.impulse_3 = shake.impulse_6 = shake.impulse_8 = 0;


    // Determine what it looks like we're doing based on the latest sample...
    uint16_t g = instantaneousPosture();

    if (g == MICROBIT_ACCELEROMETER_EVT_SHAKE)
    {
        lastGesture = MICROBIT_ACCELEROMETER_EVT_SHAKE;
        MicroBitEvent e(MICROBIT_ID_GESTURE, MICROBIT_ACCELEROMETER_EVT_SHAKE);
        return;
    }

    // Perform some low pass filtering to reduce jitter from any detected effects
    if (g == currentGesture)
    {
        if (sigma < MICROBIT_ACCELEROMETER_GESTURE_DAMPING)
            sigma++;
    }
    else
    {
        currentGesture = g;
        sigma = 0;
    }

    // If we've reached threshold, update our record and raise the relevant event...
    if (currentGesture != lastGesture && sigma >= MICROBIT_ACCELEROMETER_GESTURE_DAMPING)
    {
        lastGesture = currentGesture;
        MicroBitEvent e(MICROBIT_ID_GESTURE, lastGesture);
    }
}

/**
  * Attempts to set the sample rate of the accelerometer to the specified value (in ms).
  *
  * @param period the requested time between samples, in milliseconds.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
  *
  * @code
  * // sample rate is now 20 ms.
  * accelerometer.setPeriod(20);
  * @endcode
  *
  * @note The requested rate may not be possible on the hardware. In this case, the
  * nearest lower rate is chosen.
  */
int MicroBitAccelerometer::setPeriod(int period)
{
    int result;

    samplePeriod = period;
    result = configure();

    samplePeriod = getPeriod();
    return result;

}

/**
  * Reads the currently configured sample rate of the accelerometer.
  *
  * @return The time between samples, in milliseconds.
  */
int MicroBitAccelerometer::getPeriod()
{
    return (int)samplePeriod;
}

/**
  * Attempts to set the sample range of the accelerometer to the specified value (in g).
  *
  * @param range The requested sample range of samples, in g.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
  *
  * @code
  * // the sample range of the accelerometer is now 8G.
  * accelerometer.setRange(8);
  * @endcode
  *
  * @note The requested range may not be possible on the hardware. In this case, the
  * nearest lower range is chosen.
  */
int MicroBitAccelerometer::setRange(int range)
{
    int result;

    sampleRange = range;
    result = configure();

    sampleRange = getRange();
    return result;
}

/**
  * Reads the currently configured sample range of the accelerometer.
  *
  * @return The sample range, in g.
  */
int MicroBitAccelerometer::getRange()
{
    return (int)sampleRange;
}

/**
 * Configures the accelerometer for G range and sample rate defined
 * in this object. The nearest values are chosen to those defined
 * that are supported by the hardware. The instance variables are then
 * updated to reflect reality.
 *
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the accelerometer could not be configured.
 *
 * @note This method should be overidden by the hardware driver to implement the requested
 * changes in hardware.
 */
int MicroBitAccelerometer::configure()
{
    return MICROBIT_NOT_SUPPORTED;
}

/**
 * Poll to see if new data is available from the hardware. If so, update it.
 * n.b. it is not necessary to explicitly call this funciton to update data
 * (it normally happens in the background when the scheduler is idle), but a check is performed
 * if the user explicitly requests up to date data.
 *
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the update fails.
 *
 * @note This method should be overidden by the hardware driver to implement the requested
 * changes in hardware.
 */
int MicroBitAccelerometer::requestUpdate()
{
    microbit_panic(MICROBIT_HARDWARE_UNAVAILABLE_ACC);
    return MICROBIT_NOT_SUPPORTED;
}

/**
 * Reads the last accelerometer value stored, and provides it in the coordinate system requested.
 *
 * @param coordinateSpace The coordinate system to use.
 * @return The force measured in each axis, in milli-g.
 */
Sample3D MicroBitAccelerometer::getSample(CoordinateSystem coordinateSystem)
{
    requestUpdate();
    return coordinateSpace.transform(sampleENU, coordinateSystem);
}

/**
 * Reads the last accelerometer value stored, and in the coordinate system defined in the constructor.
 * @return The force measured in each axis, in milli-g.
 */
Sample3D MicroBitAccelerometer::getSample()
{
    requestUpdate();
    return sample;
}

/**
 * reads the value of the x axis from the latest update retrieved from the accelerometer,
 * usingthe default coordinate system as specified in the constructor.
 *
 * @return the force measured in the x axis, in milli-g.
 */
int MicroBitAccelerometer::getX()
{
    requestUpdate();
    return sample.x;
}

/**
 * reads the value of the y axis from the latest update retrieved from the accelerometer,
 * usingthe default coordinate system as specified in the constructor.
 *
 * @return the force measured in the y axis, in milli-g.
 */
int MicroBitAccelerometer::getY()
{
    requestUpdate();
    return sample.y;
}

/**
 * reads the value of the z axis from the latest update retrieved from the accelerometer,
 * usingthe default coordinate system as specified in the constructor.
 *
 * @return the force measured in the z axis, in milli-g.
 */
int MicroBitAccelerometer::getZ()
{
    requestUpdate();
    return sample.z;
}

/**
  * Provides a rotation compensated pitch of the device, based on the latest update retrieved from the accelerometer.
  *
  * @return The pitch of the device, in degrees.
  *
  * @code
  * accelerometer.getPitch();
  * @endcode
  */
int MicroBitAccelerometer::getPitch()
{
    return (int) ((360*getPitchRadians()) / (2*PI));
}

/**
  * Provides a rotation compensated pitch of the device, based on the latest update retrieved from the accelerometer.
  *
  * @return The pitch of the device, in radians.
  *
  * @code
  * accelerometer.getPitchRadians();
  * @endcode
  */
float MicroBitAccelerometer::getPitchRadians()
{
    requestUpdate();
    if (!(status & MICROBIT_ACCELEROMETER_IMU_DATA_VALID))
        recalculatePitchRoll();

    return pitch;
}

/**
  * Provides a rotation compensated roll of the device, based on the latest update retrieved from the accelerometer.
  *
  * @return The roll of the device, in degrees.
  *
  * @code
  * accelerometer.getRoll();
  * @endcode
  */
int MicroBitAccelerometer::getRoll()
{
    return (int) ((360*getRollRadians()) / (2*PI));
}

/**
  * Provides a rotation compensated roll of the device, based on the latest update retrieved from the accelerometer.
  *
  * @return The roll of the device, in radians.
  *
  * @code
  * accelerometer.getRollRadians();
  * @endcode
  */
float MicroBitAccelerometer::getRollRadians()
{
    requestUpdate();
    if (!(status & MICROBIT_ACCELEROMETER_IMU_DATA_VALID))
        recalculatePitchRoll();

    return roll;
}

/**
  * Recalculate roll and pitch values for the current sample.
  *
  * @note We only do this at most once per sample, as the necessary trigonemteric functions are rather
  *       heavyweight for a CPU without a floating point unit.
  */
void MicroBitAccelerometer::recalculatePitchRoll()
{
    double x = (double) sample.x;
    double y = (double) sample.y;
    double z = (double) sample.z;

    roll = atan2(x, -z);
    pitch = atan2(y, (x*sin(roll) - z*cos(roll)));

#if CONFIG_ENABLED(MICROBIT_FULL_RANGE_PITCH_CALCULATION)

    // Handle to the two "negative quadrants", such that we get an output in the +/- 18- degree range.
    // This ensures that the pitch values are consistent with the roll values.
    if (z > 0.0)
    {
        double reference = pitch > 0.0 ? (PI / 2.0) : (-PI / 2.0);
        pitch = reference + (reference - pitch);
    }
#endif

    status |= MICROBIT_ACCELEROMETER_IMU_DATA_VALID;
}

/**
  * Retrieves the last recorded gesture.
  *
  * @return The last gesture that was detected.
  *
  * Example:
  * @code
  *
  * if (accelerometer.getGesture() == SHAKE)
  *     display.scroll("SHAKE!");
  * @endcode
  */
uint16_t MicroBitAccelerometer::getGesture()
{
    return lastGesture;
}

/**
  * Destructor for FXS8700, where we deregister from the array of fiber components.
  */
MicroBitAccelerometer::~MicroBitAccelerometer()
{
}

MicroBitAccelerometer* MicroBitAccelerometer::detectedAccelerometer = NULL;
