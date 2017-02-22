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

#ifdef TARGET_NRF51_MICROBIT

/**
 * Class definition for MicroBit Accelerometer.
 *
 * Represents an implementation of the Freescale MMA8653 3 axis accelerometer
 * Also includes basic data caching and on demand activation.
 */
#include "MicroBitConfig.h"
#include "MicroBitAccelerometer.h"
#include "ErrorNo.h"
#include "MicroBitConfig.h"
#include "MicroBitEvent.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"

/**
  * Configures the accelerometer for G range and sample rate defined
  * in this object. The nearest values are chosen to those defined
  * that are supported by the hardware. The instance variables are then
  * updated to reflect reality.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the accelerometer could not be configured.
  */
int MicroBitAccelerometer::configure()
{
    const MMA8653SampleRangeConfig  *actualSampleRange;
    const MMA8653SampleRateConfig  *actualSampleRate;
    int result;

    // First find the nearest sample rate to that specified.
    actualSampleRate = &MMA8653SampleRate[MMA8653_SAMPLE_RATES-1];
    for (int i=MMA8653_SAMPLE_RATES-1; i>=0; i--)
    {
        if(MMA8653SampleRate[i].sample_period < this->samplePeriod * 1000)
            break;

        actualSampleRate = &MMA8653SampleRate[i];
    }

    // Now find the nearest sample range to that specified.
    actualSampleRange = &MMA8653SampleRange[MMA8653_SAMPLE_RANGES-1];
    for (int i=MMA8653_SAMPLE_RANGES-1; i>=0; i--)
    {
        if(MMA8653SampleRange[i].sample_range < this->sampleRange)
            break;

        actualSampleRange = &MMA8653SampleRange[i];
    }

    // OK, we have the correct data. Update our local state.
    this->samplePeriod = actualSampleRate->sample_period / 1000;
    this->sampleRange = actualSampleRange->sample_range;

    // Now configure the accelerometer accordingly.
    // First place the device into standby mode, so it can be configured.
    result = writeCommand(MMA8653_CTRL_REG1, 0x00);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Enable high precisiosn mode. This consumes a bit more power, but still only 184 uA!
    result = writeCommand(MMA8653_CTRL_REG2, 0x10);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Enable the INT1 interrupt pin.
    result = writeCommand(MMA8653_CTRL_REG4, 0x01);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Select the DATA_READY event source to be routed to INT1
    result = writeCommand(MMA8653_CTRL_REG5, 0x01);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Configure for the selected g range.
    result = writeCommand(MMA8653_XYZ_DATA_CFG, actualSampleRange->xyz_data_cfg);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Bring the device back online, with 10bit wide samples at the requested frequency.
    result = writeCommand(MMA8653_CTRL_REG1, actualSampleRate->ctrl_reg1 | 0x01);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    return MICROBIT_OK;
}

/**
  * Issues a standard, 2 byte I2C command write to the accelerometer.
  *
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to write to.
  *
  * @param value The value to write.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the the write request failed.
  */
int MicroBitAccelerometer::writeCommand(uint8_t reg, uint8_t value)
{
    uint8_t command[2];
    command[0] = reg;
    command[1] = value;

    return i2c.write(address, (const char *)command, 2);
}

/**
  * Issues a read command, copying data into the specified buffer.
  *
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to access.
  *
  * @param buffer Memory area to read the data into.
  *
  * @param length The number of bytes to read.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER or MICROBIT_I2C_ERROR if the the read request failed.
  */
int MicroBitAccelerometer::readCommand(uint8_t reg, uint8_t* buffer, int length)
{
    int result;

    if (buffer == NULL || length <= 0 )
        return MICROBIT_INVALID_PARAMETER;

    result = i2c.write(address, (const char *)&reg, 1, true);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    result = i2c.read(address, (char *)buffer, length);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    return MICROBIT_OK;
}

/**
  * Constructor.
  * Create a software abstraction of an accelerometer.
  *
  * @param _i2c an instance of MicroBitI2C used to communicate with the onboard accelerometer.
  *
  * @param address the default I2C address of the accelerometer. Defaults to: MMA8653_DEFAULT_ADDR.
  *
  * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_ACCELEROMETER
  *
  * @code
  * MicroBitI2C i2c = MicroBitI2C(I2C_SDA0, I2C_SCL0);
  *
  * MicroBitAccelerometer accelerometer = MicroBitAccelerometer(i2c);
  * @endcode
 */
MicroBitAccelerometer::MicroBitAccelerometer(MicroBitI2C& _i2c, uint16_t address, uint16_t id) : sample(), int1(MICROBIT_PIN_ACCEL_DATA_READY), i2c(_i2c)
{
    // Store our identifiers.
    this->id = id;
    this->status = 0;
    this->address = address;

    // Update our internal state for 50Hz at +/- 2g (50Hz has a period af 20ms).
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

    // Configure and enable the accelerometer.
    if (this->configure() == MICROBIT_OK)
        status |= MICROBIT_COMPONENT_RUNNING;
}

/**
  * Attempts to read the 8 bit ID from the accelerometer, this can be used for
  * validation purposes.
  *
  * @return the 8 bit ID returned by the accelerometer, or MICROBIT_I2C_ERROR if the request fails.
  *
  * @code
  * accelerometer.whoAmI();
  * @endcode
  */
int MicroBitAccelerometer::whoAmI()
{
    uint8_t data;
    int result;

    result = readCommand(MMA8653_WHOAMI, &data, 1);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    return (int)data;
}

/**
  * Reads the acceleration data from the accelerometer, and stores it in our buffer.
  * This only happens if the accelerometer indicates that it has new data via int1.
  *
  * On first use, this member function will attempt to add this component to the
  * list of fiber components in order to constantly update the values stored
  * by this object.
  *
  * This technique is called lazy instantiation, and it means that we do not
  * obtain the overhead from non-chalantly adding this component to fiber components.
  *
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the read request fails.
  */
int MicroBitAccelerometer::updateSample()
{
    if(!(status & MICROBIT_ACCEL_ADDED_TO_IDLE))
    {
        fiber_add_idle_component(this);
        status |= MICROBIT_ACCEL_ADDED_TO_IDLE;
    }

    // Poll interrupt line from accelerometer.
    // n.b. Default is Active LO. Interrupt is cleared in data read.
    if(!int1)
    {
        int8_t data[6];
        int result;

        result = readCommand(MMA8653_OUT_X_MSB, (uint8_t *)data, 6);
        if (result !=0)
            return MICROBIT_I2C_ERROR;

        // read MSB values...
        sample.x = data[0];
        sample.y = data[2];
        sample.z = data[4];

        // Normalize the data in the 0..1024 range.
        sample.x *= 8;
        sample.y *= 8;
        sample.z *= 8;

#if CONFIG_ENABLED(USE_ACCEL_LSB)
        // Add in LSB values.
        sample.x += (data[1] / 64);
        sample.y += (data[3] / 64);
        sample.z += (data[5] / 64);
#endif

        // Scale into millig (approx!)
        sample.x *= this->sampleRange;
        sample.y *= this->sampleRange;
        sample.z *= this->sampleRange;

        // Indicate that pitch and roll data is now stale, and needs to be recalculated if needed.
        status &= ~MICROBIT_ACCEL_PITCH_ROLL_VALID;

        // Update gesture tracking
        updateGesture();

        // Indicate that a new sample is available
        MicroBitEvent e(id, MICROBIT_ACCELEROMETER_EVT_DATA_UPDATE);
    }

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
int MicroBitAccelerometer::instantaneousAccelerationSquared()
{
    updateSample();

    // Use pythagoras theorem to determine the combined force acting on the device.
    return (int)sample.x*(int)sample.x + (int)sample.y*(int)sample.y + (int)sample.z*(int)sample.z;
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
    if ((getX() < -MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && shake.x) || (getX() > MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && !shake.x))
    {
        shakeDetected = true;
        shake.x = !shake.x;
    }

    if ((getY() < -MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && shake.y) || (getY() > MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && !shake.y))
    {
        shakeDetected = true;
        shake.y = !shake.y;
    }

    if ((getZ() < -MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && shake.z) || (getZ() > MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE && !shake.z))
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

    if (instantaneousAccelerationSquared() < MICROBIT_ACCELEROMETER_FREEFALL_THRESHOLD)
        return MICROBIT_ACCELEROMETER_EVT_FREEFALL;

    // Determine our posture.
    if (getX() < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_LEFT;

    if (getX() > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_RIGHT;

    if (getY() < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_DOWN;

    if (getY() > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_TILT_UP;

    if (getZ() < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return MICROBIT_ACCELEROMETER_EVT_FACE_UP;

    if (getZ() > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
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
    int force = instantaneousAccelerationSquared();

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
    this->samplePeriod = period;
    return this->configure();
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
    this->sampleRange = range;
    return this->configure();
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
  * Reads the value of the X axis from the latest update retrieved from the accelerometer.
  *
  * @param system The coordinate system to use. By default, a simple cartesian system is provided.
  *
  * @return The force measured in the X axis, in milli-g.
  *
  * @code
  * accelerometer.getX();
  * @endcode
  */
int MicroBitAccelerometer::getX(MicroBitCoordinateSystem system)
{
    updateSample();

    switch (system)
    {
        case SIMPLE_CARTESIAN:
            return -sample.x;

        case NORTH_EAST_DOWN:
            return sample.y;

        case RAW:
        default:
            return sample.x;
    }
}

/**
  * Reads the value of the Y axis from the latest update retrieved from the accelerometer.
  *
  * @return The force measured in the Y axis, in milli-g.
  *
  * @code
  * accelerometer.getY();
  * @endcode
  */
int MicroBitAccelerometer::getY(MicroBitCoordinateSystem system)
{
    updateSample();

    switch (system)
    {
        case SIMPLE_CARTESIAN:
            return -sample.y;

        case NORTH_EAST_DOWN:
            return -sample.x;

        case RAW:
        default:
            return sample.y;
    }
}

/**
  * Reads the value of the Z axis from the latest update retrieved from the accelerometer.
  *
  * @return The force measured in the Z axis, in milli-g.
  *
  * @code
  * accelerometer.getZ();
  * @endcode
  */
int MicroBitAccelerometer::getZ(MicroBitCoordinateSystem system)
{
    updateSample();

    switch (system)
    {
        case NORTH_EAST_DOWN:
            return -sample.z;

        case SIMPLE_CARTESIAN:
        case RAW:
        default:
            return sample.z;
    }
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
    if (!(status & MICROBIT_ACCEL_PITCH_ROLL_VALID))
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
    if (!(status & MICROBIT_ACCEL_PITCH_ROLL_VALID))
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
    double x = (double) getX(NORTH_EAST_DOWN);
    double y = (double) getY(NORTH_EAST_DOWN);
    double z = (double) getZ(NORTH_EAST_DOWN);

    roll = atan2(y, z);
    pitch = atan(-x / (y*sin(roll) + z*cos(roll)));

    status |= MICROBIT_ACCEL_PITCH_ROLL_VALID;
}

/**
  * Retrieves the last recorded gesture.
  *
  * @return The last gesture that was detected.
  *
  * Example:
  * @code
  * MicroBitDisplay display;
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
  * A periodic callback invoked by the fiber scheduler idle thread.
  *
  * Internally calls updateSample().
  */
void MicroBitAccelerometer::idleTick()
{
    updateSample();
}

/**
  * Destructor for MicroBitAccelerometer, where we deregister from the array of fiber components.
  */
MicroBitAccelerometer::~MicroBitAccelerometer()
{
    fiber_remove_idle_component(this);
}

const MMA8653SampleRangeConfig MMA8653SampleRange[MMA8653_SAMPLE_RANGES] = {
    {2, 0},
    {4, 1},
    {8, 2}
};

const MMA8653SampleRateConfig MMA8653SampleRate[MMA8653_SAMPLE_RATES] = {
    {1250,      0x00},
    {2500,      0x08},
    {5000,      0x10},
    {10000,     0x18},
    {20000,     0x20},
    {80000,     0x28},
    {160000,    0x30},
    {640000,    0x38}
};
#endif