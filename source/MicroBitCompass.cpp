#include "MicroBit.h"

/**
  * Constructor.
  * Create a compass representation with the given ID.
  * @param id the event ID of the compass object.
  * @param address the default address for the compass register
  *
  * Example:
  * @code
  * compass(MICROBIT_ID_COMPASS, MAG3110_DEFAULT_ADDR);
  * @endcode
  *
  * Possible Events for the compass are as follows:
  * @code
  * MICROBIT_COMPASS_EVT_CAL_REQUIRED   // triggered when no magnetometer data is available in persistent storage
  * MICROBIT_COMPASS_EVT_CAL_START      // triggered when calibration has begun
  * MICROBIT_COMPASS_EVT_CAL_END        // triggered when calibration has finished.
  * @endcode
  */
MicroBitCompass::MicroBitCompass(uint16_t id, uint16_t address) : average(), sample(), int1(MICROBIT_PIN_COMPASS_DATA_READY)
{
    this->id = id;
    this->address = address;

    // We presume the device calibrated until the average values are read.
    this->status = 0x01;

    // Select 10Hz update rate, with oversampling, and enable the device.
    this->samplePeriod = 100;
    this->configure();

    // Assume that we have no calibraiton information.
    status &= ~MICROBIT_COMPASS_STATUS_CALIBRATED;

    // Indicate that we're up and running.
    uBit.flags |= MICROBIT_FLAG_COMPASS_RUNNING;
}

/**
  * Issues a standard, 2 byte I2C command write to the magnetometer.
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to write to.
  * @param value The value to write.
  * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
  */
int MicroBitCompass::writeCommand(uint8_t reg, uint8_t value)
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
  */
int MicroBitCompass::readCommand(uint8_t reg, uint8_t* buffer, int length)
{
    int result;

    if (buffer == NULL || length <= 0)
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
  * Issues a read of a given address, and returns the value.
  * Blocks the calling thread until complete.
  *
  * @param reg The based address of the 16 bit register to access.
  * @return The register value, interpreted as a 16 but signed value, or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
  */
int MicroBitCompass::read16(uint8_t reg)
{
    uint8_t cmd[2];
    int result;

    cmd[0] = reg;
    result = uBit.i2c.write(address, (const char *)cmd, 1);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    cmd[0] = 0x00;
    cmd[1] = 0x00;

    result = uBit.i2c.read(address, (char *)cmd, 2);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    return (int16_t) ((cmd[1] | (cmd[0] << 8))); //concatenate the MSB and LSB
}

/**
  * Issues a read of a given address, and returns the value.
  * Blocks the calling thread until complete.
  *
  * @param reg The based address of the 16 bit register to access.
  * @return The register value, interpreted as a 8 bit unsigned value, or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
  */
int MicroBitCompass::read8(uint8_t reg)
{
    uint8_t data;
    int result;

    data = 0;
    result = readCommand(reg, (uint8_t*) &data, 1);
    if (result != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;

    return data;
}

/**
 * Gets the current heading of the device, relative to magnetic north.
 * If the compass is not calibrated, it will raise the MICROBIT_COMPASS_EVT_CALIBRATE event.
 * Users wishing to implement their own calibration algorithms should listen for this event,
 * using MESSAGE_BUS_LISTENER_IMMEDIATE model. This ensures that calibration is complete before
 * the user program continues.
 *
 * @return the current heading, in degrees. Or MICROBIT_CALIBRATION_IN_PROGRESS if the compass is calibrating.
 *
 * Example:
 * @code
 * uBit.compass.heading();
 * @endcode
 */
int MicroBitCompass::heading()
{
    float bearing;

    if(status & MICROBIT_COMPASS_STATUS_CALIBRATING)
        return MICROBIT_CALIBRATION_IN_PROGRESS;

    if(!(status & MICROBIT_COMPASS_STATUS_CALIBRATED))
        calibrate();

    // Precompute the tilt compensation parameters to improve readability.
    float phi = uBit.accelerometer.getRollRadians();
    float theta = uBit.accelerometer.getPitchRadians();
    float x = (float) getX(NORTH_EAST_DOWN);
    float y = (float) getY(NORTH_EAST_DOWN);
    float z = (float) getZ(NORTH_EAST_DOWN);

    // Precompute cos and sin of pitch and roll angles to make the calculation a little more efficient.
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);

    bearing = (360*atan2(z*sinPhi - y*cosPhi, x*cosTheta + y*sinTheta*sinPhi + z*sinTheta*cosPhi)) / (2*PI);

    if (bearing < 0)
        bearing += 360.0;

    return (int) bearing;
}

/**
  * Periodic callback from MicroBit clock.
  * Check if any data is ready for reading by checking the interrupt.
  */
void MicroBitCompass::idleTick()
{
    // Poll interrupt line from accelerometer (Active HI).
    // Interrupt is cleared on data read of MAG_OUT_X_MSB.
    if(int1)
    {
        sample.x = MAG3110_NORMALIZE_SAMPLE((int) read16(MAG_OUT_X_MSB));
        sample.y = MAG3110_NORMALIZE_SAMPLE((int) read16(MAG_OUT_Y_MSB));
        sample.z = MAG3110_NORMALIZE_SAMPLE((int) read16(MAG_OUT_Z_MSB));

        // Indicate that a new sample is available
        MicroBitEvent e(id, MICROBIT_COMPASS_EVT_DATA_UPDATE);
    }
}

/**
  * Reads the X axis value of the latest update from the compass.
  * @return The magnetic force measured in the X axis, in nano teslas.
  *
  * Example:
  * @code
  * uBit.compass.getX();
  * @endcode
  */
int MicroBitCompass::getX(MicroBitCoordinateSystem system)
{
    switch (system)
    {
        case SIMPLE_CARTESIAN:
            return sample.x - average.x;

        case NORTH_EAST_DOWN:
            return -(sample.y - average.y);

        case RAW:
        default:
            return sample.x;
    }
}

/**
  * Reads the Y axis value of the latest update from the compass.
  * @return The magnetic force measured in the Y axis, in nano teslas.
  *
  * Example:
  * @code
  * uBit.compass.getY();
  * @endcode
  */
int MicroBitCompass::getY(MicroBitCoordinateSystem system)
{
    switch (system)
    {
        case SIMPLE_CARTESIAN:
            return -(sample.y - average.y);

        case NORTH_EAST_DOWN:
            return (sample.x - average.x);

        case RAW:
        default:
            return sample.y;
    }
}

/**
  * Reads the Z axis value of the latest update from the compass.
  * @return The magnetic force measured in the Z axis, in nano teslas.
  *
  * Example:
  * @code
  * uBit.compass.getZ();
  * @endcode
  */
int MicroBitCompass::getZ(MicroBitCoordinateSystem system)
{
    switch (system)
    {
        case SIMPLE_CARTESIAN:
        case NORTH_EAST_DOWN:
            return -(sample.z - average.z);

        case RAW:
        default:
            return sample.z;
    }
}

/**
  * Determines the overall magnetic field strength based on the latest update from the compass.
  * @return The magnetic force measured across all axes, in nano teslas.
  *
  * Example:
  * @code
  * uBit.compass.getFieldStrength();
  * @endcode
  */
int MicroBitCompass::getFieldStrength()
{
    double x = getX();
    double y = getY();
    double z = getZ();

    return (int) sqrt(x*x + y*y + z*z);
}

/**
 * Configures the compass for the sample rate defined
 * in this object. The nearest values are chosen to those defined
 * that are supported by the hardware. The instance variables are then
 * updated to reflect reality.
 * @return MICROBIT_OK or MICROBIT_I2C_ERROR if the magnetometer could not be configured.
 */
int MicroBitCompass::configure()
{
    const MAG3110SampleRateConfig  *actualSampleRate;
    int result;

    // First, take the device offline, so it can be configured.
    result = writeCommand(MAG_CTRL_REG1, 0x00);
    if (result != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;

    // Wait for the part to enter standby mode...
    while(1)
    {
        // Read the status of the part...
        // If we can't communicate with it over I2C, pass on the error.
        result = this->read8(MAG_SYSMOD);
        if (result == MICROBIT_I2C_ERROR)
            return MICROBIT_I2C_ERROR;

        // if the part in in standby, we're good to carry on.
        if((result & 0x03) == 0)
            break;

        // Perform a power efficient sleep...
        uBit.sleep(100);
    }

    // Find the nearest sample rate to that specified.
    actualSampleRate = &MAG3110SampleRate[MAG3110_SAMPLE_RATES-1];
    for (int i=MAG3110_SAMPLE_RATES-1; i>=0; i--)
    {
        if(MAG3110SampleRate[i].sample_period < this->samplePeriod * 1000)
            break;

        actualSampleRate = &MAG3110SampleRate[i];
    }

    // OK, we have the correct data. Update our local state.
    this->samplePeriod = actualSampleRate->sample_period / 1000;

    // Enable automatic reset after each sample;
    result = writeCommand(MAG_CTRL_REG2, 0xA0);
    if (result != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;


    // Bring the device online, with the requested sample frequency.
    result = writeCommand(MAG_CTRL_REG1, actualSampleRate->ctrl_reg1 | 0x01);
    if (result != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;

    return MICROBIT_OK;
}

/**
 * Attempts to set the sample rate of the compass to the specified value (in ms).
 * n.b. the requested rate may not be possible on the hardware. In this case, the
 * nearest lower rate is chosen.
 * @param period the requested time between samples, in milliseconds.
 * @return MICROBIT_OK or MICROBIT_I2C_ERROR if the magnetometer could not be updated.
 */
int MicroBitCompass::setPeriod(int period)
{
    this->samplePeriod = period;
    return this->configure();
}

/**
 * Reads the currently configured sample rate of the compass.
 * @return The time between samples, in milliseconds.
 */
int MicroBitCompass::getPeriod()
{
    return (int)samplePeriod;
}


/**
  * Attempts to determine the 8 bit ID from the magnetometer.
  * @return the id of the compass (magnetometer), or MICROBIT_I2C_ERROR if the magnetometer could not be updated.
  *
  * Example:
  * @code
  * uBit.compass.whoAmI();
  * @endcode
  */
int MicroBitCompass::whoAmI()
{
    uint8_t data;
    int result;

    result = readCommand(MAG_WHOAMI, &data, 1);
    if (result != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;

    return (int)data;
}

/**
 * Reads the current die temperature of the compass.
 * @return the temperature in degrees celsius, or MICROBIT_I2C_ERROR if the magnetometer could not be updated.
 */
int MicroBitCompass::readTemperature()
{
    int8_t temperature;
    int result;

    result = readCommand(MAG_DIE_TEMP, (uint8_t *)&temperature, 1);
    if (result != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;

    return temperature;
}

/**
  * Perform a calibration of the compass.
  *
  * This method will be called automatically if a user attempts to read a compass value when
  * the compass is uncalibrated. It can also be called at any time by the user.
  *
  * Any old calibration data is deleted.
  * The method will only return once the compass has been calibrated.
  *
  * @return MICROBIT_OK, MICROBIT_I2C_ERROR if the magnetometer could not be accessed,
  * or MICROBIT_CALIBRATION_REQUIRED if the calibration algorithm failed to complete succesfully.
  * @note THIS MUST BE CALLED TO GAIN RELIABLE VALUES FROM THE COMPASS
  */
int MicroBitCompass::calibrate()
{
    // Only perform one calibration process at a time.
    if(isCalibrating())
        return MICROBIT_CALIBRATION_IN_PROGRESS;

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
  * Perform a calibration of the compass.
  * This will fire MICROBIT_COMPASS_EVT_CAL_START.
  * @return MICROBIT_OK, or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
  *
  * @note *** THIS FUNCTION IS NOW DEPRECATED AND WILL BE REMOVED IN THE NEXT MAJOR RELEASE ***
  * @note *** PLEASE USE THE calibrate() FUNCTION INSTEAD ***
  */
int MicroBitCompass::calibrateStart()
{
    return calibrate();
}

/**
 * Perform the asynchronous calibration of the compass.
 * This will fire MICROBIT_COMPASS_EVT_CAL_START and MICROBIT_COMPASS_EVT_CAL_END when finished.
 *
 * @note *** THIS FUNCITON IS NOW DEPRECATED AND WILL BE REMOVED IN THE NEXT MAJOR RELEASE ***
 * @note *** PLEASE USE THE calibrate() FUNCTION INSTEAD ***
 */
void MicroBitCompass::calibrateAsync()
{
    calibrate();
}

/**
  * Complete the calibration of the compass.
  * This will fire MICROBIT_COMPASS_EVT_CAL_END.
  *
  * @note *** THIS FUNCTION IS NOW DEPRECATED AND WILL BE REMOVED IN THE NEXT MAJOR RELEASE ***
  */
void MicroBitCompass::calibrateEnd()
{
}


/**
  * Configure the compass to use the given calibration data.
  * Calibration data is comprised of the perceived zero offset of each axis of the compass.
  * After calibration this should now take into account trimming errors in the magnetometer,
  * and any "hard iron" offsets on the device.
  *
  * @param The x, y and z zero offsets to use as calibration data.
  */
void MicroBitCompass::setCalibration(CompassSample calibration)
{

    MicroBitStorage s = MicroBitStorage();
    MicroBitConfigurationBlock *b = s.getConfigurationBlock();

    //check we are not storing our restored calibration data.
    if(b->compassCalibrationData != calibration)
    {
        b->magic = MICROBIT_STORAGE_CONFIG_MAGIC;

        b->compassCalibrationData.x = calibration.x;
        b->compassCalibrationData.y = calibration.y;
        b->compassCalibrationData.z = calibration.z;

        s.setConfigurationBlock(b);
    }

    delete b;

    average = calibration;
    status |= MICROBIT_COMPASS_STATUS_CALIBRATED;
}

/**
  * Provides the calibration data currently in use by the compass.
  * More specifically, the x, y and z zero offsets of the compass.
  *
  * @return The x, y and z xero offsets of the compass.
  */
CompassSample MicroBitCompass::getCalibration()
{
    return average;
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
  * Clears the calibration held in persistent storage, and sets the calibrated flag to zero.
  */
void MicroBitCompass::clearCalibration()
{
    status &= ~MICROBIT_COMPASS_STATUS_CALIBRATED;
}

/**
  * Returns 0 or 1. 1 indicates data is waiting to be read, zero means data is not ready to be read.
  */
int MicroBitCompass::isIdleCallbackNeeded()
{
    // The MAG3110 raises an interrupt line when data is ready, which we sample here.
    // The interrupt line is active HI, so simply return the state of the pin.
    return int1;
}

/**
  * Destructor for MicroBitMessageBus, so that we deregister ourselves as an idleComponent
  */
MicroBitCompass::~MicroBitCompass()
{
    uBit.removeIdleComponent(this);
}

const MAG3110SampleRateConfig MAG3110SampleRate[MAG3110_SAMPLE_RATES] = {
    {12500,      0x00},        // 80 Hz
    {25000,      0x20},        // 40 Hz
    {50000,      0x40},        // 20 Hz
    {100000,     0x60},        // 10 hz
    {200000,     0x80},        // 5 hz
    {400000,     0x88},        // 2.5 hz
    {800000,     0x90},        // 1.25 hz
    {1600000,    0xb0},        // 0.63 hz
    {3200000,    0xd0},        // 0.31 hz
    {6400000,    0xf0},        // 0.16 hz
    {12800000,   0xf8}         // 0.08 hz
};
