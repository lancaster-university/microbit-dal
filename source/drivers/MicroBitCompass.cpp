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

#include "MicroBitCompass.h"
#include "ErrorNo.h"
#include "MicroBitEvent.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"
#include "MicroBitDevice.h"

#include "MAG3110.h"
#include "LSM303Magnetometer.h"
#include "FXOS8700.h"

//
// Internal convenience macro to apply calibration to a given sample.
//
#define CALIBRATED_SAMPLE(sample, axis) (((sample.axis - calibration.centre.axis) * calibration.scale.axis) >> 10)

/**
 * Constructor.
 * Create a software abstraction of an e-compass.
 *
 * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_COMPASS
 * @param coordinateSpace the orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
 *
 */
MicroBitCompass::MicroBitCompass(CoordinateSpace &cspace, uint16_t id) : calibration(), sample(), sampleENU(), coordinateSpace(cspace)
{
    accelerometer = NULL;
    init(id);
}

/**
 * Constructor.
 * Create a software abstraction of an e-compass.
 *
 * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_COMPASS
 * @param accel the accelerometer to use for tilt compensation
 * @param coordinateSpace the orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
 *
 */
MicroBitCompass::MicroBitCompass(MicroBitAccelerometer &accel, CoordinateSpace &cspace, uint16_t id) :  calibration(), sample(), sampleENU(), coordinateSpace(cspace)
{
    accelerometer = &accel;
    init(id);
}

/**
 * Internal helper used to de-duplicate code in the constructors
 * @param coordinateSpace the orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
 * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_COMPASS
 *
 */
void MicroBitCompass::init(uint16_t id)
{
    // Store our identifiers.
    this->id = id;
    this->status = 0;

    // Set a default rate of 10Hz.
    this->samplePeriod = 100;
    this->configure();

    // Assume that we have no calibration information.
    status &= ~MICROBIT_COMPASS_STATUS_CALIBRATED;

    // Indicate that we're up and running.
    status |= MICROBIT_COMPONENT_RUNNING;
}

/**
 * Device autodetection. Scans the given I2C bus for supported accelerometer devices.
 * if found, constructs an appropriate driver and returns it.
 *
 * @param i2c the bus to scan. 
 * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_ACCELEROMETER
 *
 */
MicroBitCompass& MicroBitCompass::autoDetect(MicroBitI2C &i2c)
{
    if (MicroBitCompass::detectedCompass == NULL)
    {
        // Configuration of IRQ lines
        MicroBitPin int1(MICROBIT_ID_IO_INT1, P0_28, PIN_CAPABILITY_STANDARD);
        MicroBitPin int2(MICROBIT_ID_IO_INT2, P0_29, PIN_CAPABILITY_STANDARD);
        MicroBitPin int3(MICROBIT_ID_IO_INT3, P0_27, PIN_CAPABILITY_STANDARD);

        // All known accelerometer/magnetometer peripherals have the same alignment
        CoordinateSpace &coordinateSpace = *(new CoordinateSpace(SIMPLE_CARTESIAN, true, COORDINATE_SPACE_ROTATED_0));

        // Now, probe for connected peripherals, if none have already been found.
        if (MAG3110::isDetected(i2c))
            MicroBitCompass::detectedCompass = new MAG3110(i2c, int2, coordinateSpace);

        else if (LSM303Magnetometer::isDetected(i2c))
            MicroBitCompass::detectedCompass = new LSM303Magnetometer(i2c, int2, coordinateSpace);

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
            MicroBitCompass *unavailable = new MicroBitCompass(coordinateSpace, MICROBIT_ID_COMPASS);
            MicroBitCompass::detectedCompass = unavailable;
        }
    }

    // If an accelerometer has been discovered, enable tilt compensation on the e-compass.
    if (MicroBitAccelerometer::detectedAccelerometer)
        MicroBitCompass::detectedCompass->setAccelerometer(*MicroBitAccelerometer::detectedAccelerometer);

    return *MicroBitCompass::detectedCompass;
}

/**
 * Gets the current heading of the device, relative to magnetic north.
 *
 * If the compass is not calibrated, it will raise the MICROBIT_COMPASS_EVT_CALIBRATE event.
 *
 * Users wishing to implement their own calibration algorithms should listen for this event,
 * using MESSAGE_BUS_LISTENER_IMMEDIATE model. This ensures that calibration is complete before
 * the user program continues.
 *
 * @return the current heading, in degrees. Or CALIBRATION_IN_PROGRESS if the compass is calibrating.
 *
 * @code
 * compass.heading();
 * @endcode
 */
int MicroBitCompass::heading()
{
    if(status & MICROBIT_COMPASS_STATUS_CALIBRATING)
        return MICROBIT_CALIBRATION_IN_PROGRESS;

    if(!(status & MICROBIT_COMPASS_STATUS_CALIBRATED))
        calibrate();

    if(accelerometer != NULL)
        return tiltCompensatedBearing();

    return basicBearing();
}

/**
 * Determines the overall magnetic field strength based on the latest update from the magnetometer.
 *
 * @return The magnetic force measured across all axis, in nano teslas.
 *
 * @code
 * compass.getFieldStrength();
 * @endcode
 */
int MicroBitCompass::getFieldStrength()
{
    Sample3D s = getSample();

    double x = s.x;
    double y = s.y;
    double z = s.z;

    return (int) sqrt(x*x + y*y + z*z);
}

/**
 * Perform a calibration of the compass.
 *
 * This method will be called automatically if a user attempts to read a compass value when
 * the compass is uncalibrated. It can also be called at any time by the user.
 *
 * The method will only return once the compass has been calibrated.
 *
 * @return MICROBIT_OK, MICROBIT_I2C_ERROR if the magnetometer could not be accessed,
 * or MICROBIT_CALIBRATION_REQUIRED if the calibration algorithm failed to complete successfully.
 *
 * @note THIS MUST BE CALLED TO GAIN RELIABLE VALUES FROM THE COMPASS
 */
int MicroBitCompass::calibrate()
{
    // Only perform one calibration process at a time.
    if(isCalibrating())
        return MICROBIT_CALIBRATION_IN_PROGRESS;

    requestUpdate();

    // Delete old calibration data
    clearCalibration();

    // Record that we've started calibrating.
    status |= MICROBIT_COMPASS_STATUS_CALIBRATING;

    // Launch any registred calibration alogrithm visialisation
    MicroBitEvent(id, MICROBIT_COMPASS_EVT_CALIBRATE);

    // Record that we've finished calibrating.
    status &= ~MICROBIT_COMPASS_STATUS_CALIBRATING;

    // If there are no changes to our sample data, we either have no calibration algorithm, or it couldn't complete succesfully.
    if(!(status & MICROBIT_COMPASS_STATUS_CALIBRATED))
        return MICROBIT_CALIBRATION_REQUIRED;

    return MICROBIT_OK;
}

/**
 * Configure the compass to use the calibration data that is supplied to this call.
 *
 * Calibration data is comprised of the perceived zero offset of each axis of the compass.
 *
 * After calibration this should now take into account trimming errors in the magnetometer,
 * and any "hard iron" offsets on the device.
 *
 * @param calibration A Sample3D containing the offsets for the x, y and z axis.
 */
void MicroBitCompass::setCalibration(CompassCalibration calibration)
{
    this->calibration = calibration;
    status |= MICROBIT_COMPASS_STATUS_CALIBRATED;
}

/**
 * Provides the calibration data currently in use by the compass.
 *
 * More specifically, the x, y and z zero offsets of the compass.
 *
 * @return A Sample3D containing the offsets for the x, y and z axis.
 */
CompassCalibration MicroBitCompass::getCalibration()
{
    return calibration;
}

/**
 * Returns 0 or 1. 1 indicates that the compass is calibrated, zero means the compass requires calibration.
 */
int MicroBitCompass::isCalibrated()
{
    return status & MICROBIT_COMPASS_STATUS_CALIBRATED;
}

/**
 * Returns 0 or 1. 1 indicates that the compass is calibrating, zero means the compass is not currently calibrating.
 */
int MicroBitCompass::isCalibrating()
{
    return status & MICROBIT_COMPASS_STATUS_CALIBRATING;
}

/**
 * Clears the calibration held in memory storage, and sets the calibrated flag to zero.
 */
void MicroBitCompass::clearCalibration()
{
    calibration = CompassCalibration();
    status &= ~MICROBIT_COMPASS_STATUS_CALIBRATED;
}

/**
 * Configures the device for the sample rate defined
 * in this object. The nearest values are chosen to those defined
 * that are supported by the hardware. The instance variables are then
 * updated to reflect reality.
 *
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the compass could not be configured.
 */
int MicroBitCompass::configure()
{
    return MICROBIT_NOT_SUPPORTED;
}

/**
 *
 * Defines the accelerometer to be used for tilt compensation.
 *
 * @param acceleromter Reference to the accelerometer to use.
 */
void MicroBitCompass::setAccelerometer(MicroBitAccelerometer &accelerometer)
{
    this->accelerometer = &accelerometer;
}

/**
 * Attempts to set the sample rate of the compass to the specified period value (in ms).
 *
 * @param period the requested time between samples, in milliseconds.
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
 *
 * @note The requested rate may not be possible on the hardware. In this case, the
 * nearest lower rate is chosen.
 *
 * @note This method should be overriden (if supported) by specific magnetometer device drivers.
 */
int MicroBitCompass::setPeriod(int period)
{
    int result;

    samplePeriod = period;
    result = configure();

    samplePeriod = getPeriod();
    return result;

}

/**
 * Reads the currently configured sample rate of the compass.
 *
 * @return The time between samples, in milliseconds.
 */
int MicroBitCompass::getPeriod()
{
    return (int)samplePeriod;
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
int MicroBitCompass::requestUpdate()
{
    microbit_panic(MICROBIT_HARDWARE_UNAVAILABLE_MAG);
    return MICROBIT_NOT_SUPPORTED;
}

/**
 * Stores data from the compass sensor in our buffer.
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
int MicroBitCompass::update()
{
    // Store the raw data, and apply any calibration data we have.
    sampleENU.x = CALIBRATED_SAMPLE(sampleENU, x);
    sampleENU.y = CALIBRATED_SAMPLE(sampleENU, y);
    sampleENU.z = CALIBRATED_SAMPLE(sampleENU, z);

    // Store the user accessible data, in the requested coordinate space, and taking into account component placement of the sensor.
    sample = coordinateSpace.transform(sampleENU);

    // Indicate that a new sample is available
    MicroBitEvent e(id, MICROBIT_COMPASS_EVT_DATA_UPDATE);

    return MICROBIT_OK;
};

/**
 * Reads the last compass value stored, and provides it in the coordinate system requested.
 *
 * @param coordinateSpace The coordinate system to use.
 * @return The force measured in each axis, in milli-g.
 */
Sample3D MicroBitCompass::getSample(CoordinateSystem coordinateSystem)
{
    requestUpdate();
    return coordinateSpace.transform(sampleENU, coordinateSystem);
}

/**
 * Reads the last compass value stored, and in the coordinate system defined in the constructor.
 * @return The force measured in each axis, in milli-g.
 */
Sample3D MicroBitCompass::getSample()
{
    requestUpdate();
    return sample;
}

/**
 * reads the value of the x axis from the latest update retrieved from the compass,
 * using the default coordinate system as specified in the constructor.
 *
 * @return the force measured in the x axis, in milli-g.
 */
int MicroBitCompass::getX()
{
    requestUpdate();
    return sample.x;
}

/**
 * reads the value of the y axis from the latest update retrieved from the compass,
 * using the default coordinate system as specified in the constructor.
 *
 * @return the force measured in the y axis, in milli-g.
 */
int MicroBitCompass::getY()
{
    requestUpdate();
    return sample.y;
}

/**
 * reads the value of the z axis from the latest update retrieved from the compass,
 * using the default coordinate system as specified in the constructor.
 *
 * @return the force measured in the z axis, in milli-g.
 */
int MicroBitCompass::getZ()
{
    requestUpdate();
    return sample.z;
}

/**
 * Calculates a tilt compensated bearing of the device, using the accelerometer.
 */
int MicroBitCompass::tiltCompensatedBearing()
{
    // Precompute the tilt compensation parameters to improve readability.
    float phi = accelerometer->getRollRadians();
    float theta = accelerometer->getPitchRadians();

    // Convert to floating point to reduce rounding errors
    Sample3D cs = this->getSample(NORTH_EAST_DOWN);
    float x = (float) cs.x;
    float y = (float) cs.y;
    float z = (float) cs.z;

    // Precompute cos and sin of pitch and roll angles to make the calculation a little more efficient.
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);

    // Calculate the tilt compensated bearing, and convert to degrees.
    float bearing = (360*atan2(x*cosTheta + y*sinTheta*sinPhi + z*sinTheta*cosPhi, z*sinPhi - y*cosPhi)) / (2*PI);

    // Handle the 90 degree offset caused by the NORTH_EAST_DOWN based calculation.
    bearing = 90 - bearing;

    // Ensure the calculated bearing is in the 0..359 degree range.
    if (bearing < 0)
        bearing += 360.0f;

    return (int) (bearing);
}

/**
 * Calculates a non-tilt compensated bearing of the device.
 */
int MicroBitCompass::basicBearing()
{
    // Convert to floating point to reduce rounding errors
    Sample3D cs = this->getSample(SIMPLE_CARTESIAN);
    float x = (float) cs.x;
    float y = (float) cs.y;

    float bearing = (atan2(x,y))*180/PI;

    if (bearing < 0)
        bearing += 360.0;

    return (int)bearing;
}

/**
  * Destructor.
  */
MicroBitCompass::~MicroBitCompass()
{
}

MicroBitCompass* MicroBitCompass::detectedCompass= NULL;
