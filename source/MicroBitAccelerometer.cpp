/**
 * Class definition for MicroBit Accelerometer.
 *
 * Represents an implementation of the Freescale MMA8653 3 axis accelerometer
 * Also includes basic data caching and on demand activation.
 */

#include "MicroBit.h"

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
 * Blocks the calling thread until complete.
 *
 * @param reg The address of the register to write to.
 * @param value The value to write.
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the the write request failed.
 */
int MicroBitAccelerometer::writeCommand(uint8_t reg, uint8_t value)
{
    uint8_t command[2];
    command[0] = reg;
    command[1] = value;

    return uBit.i2c.write(address, (const char *)command, 2);
}

/**
 * Issues a read command into the specified buffer.
 * Blocks the calling thread until complete.
 *
 * @param reg The address of the register to access.
 * @param buffer Memory area to read the data into.
 * @param length The number of bytes to read.
 * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER or MICROBIT_I2C_ERROR if the the read request failed.
 */
int MicroBitAccelerometer::readCommand(uint8_t reg, uint8_t* buffer, int length)
{
    int result;

    if (buffer == NULL || length <= 0 )
        return MICROBIT_INVALID_PARAMETER;

    result = uBit.i2c.write(address, (const char *)&reg, 1, true);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    result = uBit.i2c.read(address, (char *)buffer, length);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    return MICROBIT_OK;
}

/**
 * Constructor.
 * Create an accelerometer representation with the given ID.
 * @param id the ID of the new object.
 * @param address the default base address of the accelerometer.
 *
 * Example:
 * @code
 * accelerometer(MICROBIT_ID_ACCELEROMETER, MMA8653_DEFAULT_ADDR)
 * @endcode
 */
MicroBitAccelerometer::MicroBitAccelerometer(uint16_t id, uint16_t address) : sample(), int1(MICROBIT_PIN_ACCEL_DATA_READY)
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
    this->lastGesture = GESTURE_NONE;
    this->currentGesture = GESTURE_NONE;
    this->shake.x = 0;
    this->shake.y = 0;
    this->shake.z = 0;
    this->shake.count = 0;
    this->shake.timer = 0;

    // Configure and enable the accelerometer.
    if (this->configure() == MICROBIT_OK)
        uBit.flags |= MICROBIT_FLAG_ACCELEROMETER_RUNNING;
}

/**
 * Attempts to determine the 8 bit ID from the accelerometer.
 * @return the 8 bit ID returned by the accelerometer, or MICROBIT_I2C_ERROR if the request fails.
 *
 * Example:
 * @code
 * uBit.accelerometer.whoAmI();
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
 * This is called by the tick() member function, if the interrupt is set!
 *
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the read request fails.
 */
int MicroBitAccelerometer::update()
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

    return MICROBIT_OK;
};

/**
 * Service function. Calculates the current scalar acceleration of the device (x^2 + y^2 + z^2).
 * It does not, however, square root the result, as this is a relatively high cost operation.
 * This is left to application code should it be needed.
 *
 * @return the sum of the square of the acceleration of the device across all axes.
 */
int MicroBitAccelerometer::instantaneousAccelerationSquared()
{
    // Use pythagoras theorem to determine the combined force acting on the device.
    return (int)sample.x*(int)sample.x + (int)sample.y*(int)sample.y + (int)sample.z*(int)sample.z;
}

/**
 * Service function. Determines the best guess posture of the device based on instantaneous data.
 * This makes no use of historic data (except for shake), and forms this input to the filter implemented in updateGesture().
 *
 * @return A best guess of the current posture of the device, based on instantaneous data.
 */
BasicGesture MicroBitAccelerometer::instantaneousPosture()
{
    int force = instantaneousAccelerationSquared();
    bool shakeDetected = false;

    // Test for shake events.
    // We detect a shake by measuring zero crossings in each axis. In other words, if we see a strong acceleration to the left followed by
    // a string acceleration to the right, then we can infer a shake. Similarly, we can do this for each acxis (left/right, up/down, in/out).
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

    if (shakeDetected && shake.count < MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD && ++shake.count == MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD)
        shake.shaken = 1;

    if (++shake.timer >= MICROBIT_ACCELEROMETER_SHAKE_DAMPING)
    {
        shake.timer = 0;
        if (shake.count > 0)
        {
            if(--shake.count == 0)
                shake.shaken = 0;
        }
    }

    if (shake.shaken)
        return GESTURE_SHAKE;

    if (force < MICROBIT_ACCELEROMETER_FREEFALL_THRESHOLD)
        return GESTURE_FREEFALL;

    if (force > MICROBIT_ACCELEROMETER_3G_THRESHOLD)
        return GESTURE_3G;

    if (force > MICROBIT_ACCELEROMETER_6G_THRESHOLD)
        return GESTURE_6G;

    if (force > MICROBIT_ACCELEROMETER_8G_THRESHOLD)
        return GESTURE_8G;

    // Determine our posture.
    if (getX() < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return GESTURE_LEFT;

    if (getX() > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return GESTURE_RIGHT;

    if (getY() < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return GESTURE_DOWN;

    if (getY() > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return GESTURE_UP;

    if (getZ() < (-1000 + MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return GESTURE_FACE_UP;

    if (getZ() > (1000 - MICROBIT_ACCELEROMETER_TILT_TOLERANCE))
        return GESTURE_FACE_DOWN;

    return GESTURE_NONE;
}

void MicroBitAccelerometer::updateGesture()
{
    // Determine what it looks like we're doing based on the latest sample...
    BasicGesture g = instantaneousPosture();

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
 * n.b. the requested rate may not be possible on the hardware. In this case, the
 * nearest lower rate is chosen.
 * @param period the requested time between samples, in milliseconds.
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
 */
int MicroBitAccelerometer::setPeriod(int period)
{
    this->samplePeriod = period;
    return this->configure();
}

/**
 * Reads the currently configured sample rate of the accelerometer.
 * @return The time between samples, in milliseconds.
 */
int MicroBitAccelerometer::getPeriod()
{
    return (int)samplePeriod;
}

/**
 * Attempts to set the sample range of the accelerometer to the specified value (in g).
 * n.b. the requested range may not be possible on the hardware. In this case, the
 * nearest lower rate is chosen.
 * @param range The requested sample range of samples, in g.
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
 */
int MicroBitAccelerometer::setRange(int range)
{
    this->sampleRange = range;
    return this->configure();
}

/**
 * Reads the currently configured sample range of the accelerometer.
 * @return The sample range, in g.
 */
int MicroBitAccelerometer::getRange()
{
    return (int)sampleRange;
}

/**
  * Reads the X axis value of the latest update from the accelerometer.
  * @param system The coordinate system to use. By default, a simple cartesian system is provided.
  * @return The force measured in the X axis, in milli-g.
  *
  * Example:
  * @code
  * uBit.accelerometer.getX();
  * uBit.accelerometer.getX(RAW);
  * @endcode
  */
int MicroBitAccelerometer::getX(MicroBitCoordinateSystem system)
{
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
  * Reads the Y axis value of the latest update from the accelerometer.
  * @param system The coordinate system to use. By default, a simple cartesian system is provided.
  * @return The force measured in the Y axis, in milli-g.
  *
  * Example:
  * @code
  * uBit.accelerometer.getY();
  * uBit.accelerometer.getY(RAW);
  * @endcode
  */
int MicroBitAccelerometer::getY(MicroBitCoordinateSystem system)
{
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
  * Reads the Z axis value of the latest update from the accelerometer.
  * @param system The coordinate system to use. By default, a simple cartesian system is provided.
  * @return The force measured in the Z axis, in milli-g.
  *
  * Example:
  * @code
  * uBit.accelerometer.getZ();
  * uBit.accelerometer.getZ(RAW);
  * @endcode
  */
int MicroBitAccelerometer::getZ(MicroBitCoordinateSystem system)
{
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
  * Provides a rotation compensated pitch of the device, based on the latest update from the accelerometer.
  * @return The pitch of the device, in degrees.
  *
  * Example:
  * @code
  * uBit.accelerometer.getPitch();
  * @endcode
  */
int MicroBitAccelerometer::getPitch()
{
    return (int) ((360*getPitchRadians()) / (2*PI));
}

float MicroBitAccelerometer::getPitchRadians()
{
    if (!(status & MICROBIT_ACCEL_PITCH_ROLL_VALID))
        recalculatePitchRoll();

    return pitch;
}

/**
  * Provides a rotation compensated roll of the device, based on the latest update from the accelerometer.
  * @return The roll of the device, in degrees.
  *
  * Example:
  * @code
  * uBit.accelerometer.getRoll();
  * @endcode
  */
int MicroBitAccelerometer::getRoll()
{
    return (int) ((360*getRollRadians()) / (2*PI));
}

float MicroBitAccelerometer::getRollRadians()
{
    if (!(status & MICROBIT_ACCEL_PITCH_ROLL_VALID))
        recalculatePitchRoll();

    return roll;
}

/**
 * Recalculate roll and pitch values for the current sample.
 * We only do this at most once per sample, as the necessary trigonemteric functions are rather
 * heavyweight for a CPU without a floating point unit...
 */
void MicroBitAccelerometer::recalculatePitchRoll()
{
    float x = (float) getX(NORTH_EAST_DOWN);
    float y = (float) getY(NORTH_EAST_DOWN);
    float z = (float) getZ(NORTH_EAST_DOWN);

    roll = atan2(getY(NORTH_EAST_DOWN), getZ(NORTH_EAST_DOWN));
    pitch = atan(-x / (y*sin(roll) + z*cos(roll)));
    status |= MICROBIT_ACCEL_PITCH_ROLL_VALID;
}

/**
 * Reads the last recorded gesture detected.
 * @return The last gesture detected.
 *
 * Example:
 * @code
 * if (uBit.accelerometer.getGesture() == SHAKE)
 * @endcode
 */
BasicGesture MicroBitAccelerometer::getGesture()
{
    return lastGesture;
}

/**
 * periodic callback from MicroBit clock.
 * Check if any data is ready for reading by checking the interrupt flag on the accelerometer
 */
void MicroBitAccelerometer::idleTick()
{
    // Poll interrupt line from accelerometer.
    // n.b. Default is Active LO. Interrupt is cleared in data read.
    //
    if(!int1)
        update();
}

/**
 * Returns 0 or 1. 1 indicates data is waiting to be read, zero means data is not ready to be read.
 */
int MicroBitAccelerometer::isIdleCallbackNeeded()
{
    return !int1;
}

/**
  * Destructor for MicroBitAccelerometer, so that we deregister ourselves as an idleComponent
  */
MicroBitAccelerometer::~MicroBitAccelerometer()
{
    uBit.removeIdleComponent(this);
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
