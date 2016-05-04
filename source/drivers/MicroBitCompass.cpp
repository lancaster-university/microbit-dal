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

/**
  * Class definition for MicroBit Compass.
  *
  * Represents an implementation of the Freescale MAG3110 I2C Magnetmometer.
  * Also includes basic caching, calibration and on demand activation.
  */
#include "MicroBitConfig.h"
#include "MicroBitCompass.h"
#include "MicroBitFiber.h"
#include "ErrorNo.h"

/**
  * An initialisation member function used by the many constructors of MicroBitCompass.
  *
  * @param id the unique identifier for this compass instance.
  *
  * @param address the base address of the magnetometer on the i2c bus.
  */
void MicroBitCompass::init(uint16_t id, uint16_t address)
{
    this->id = id;
    this->address = address;

    // Select 10Hz update rate, with oversampling, and enable the device.
    this->samplePeriod = 100;
    this->configure();

    // Assume that we have no calibration information.
    status &= ~MICROBIT_COMPASS_STATUS_CALIBRATED;

    if(this->storage != NULL)
    {
        KeyValuePair *calibrationData =  storage->get("compassCal");

        if(calibrationData != NULL)
        {
            CompassSample storedSample = CompassSample();

            memcpy(&storedSample, calibrationData->value, sizeof(CompassSample));

            setCalibration(storedSample);

            delete calibrationData;
        }
    }

    // Indicate that we're up and running.
    status |= MICROBIT_COMPONENT_RUNNING;
}

/**
  * Constructor.
  * Create a software representation of an e-compass.
  *
  * @param _i2c an instance of i2c, which the compass is accessible from.
  *
  * @param _accelerometer an instance of the accelerometer, used for tilt compensation.
  *
  * @param _storage an instance of MicroBitStorage, used to persist calibration data across resets.
  *
  * @param address the default address for the compass register on the i2c bus. Defaults to MAG3110_DEFAULT_ADDR.
  *
  * @param id the ID of the new MicroBitCompass object. Defaults to MAG3110_DEFAULT_ADDR.
  *
  * @code
  * MicroBitI2C i2c(I2C_SDA0, I2C_SCL0);
  *
  * MicroBitAccelerometer accelerometer(i2c);
  *
  * MicroBitStorage storage;
  *
  * MicroBitCompass compass(i2c, accelerometer, storage);
  * @endcode
  */
MicroBitCompass::MicroBitCompass(MicroBitI2C& _i2c, MicroBitAccelerometer& _accelerometer, MicroBitStorage& _storage, uint16_t address,  uint16_t id) :
    average(),
    sample(),
    int1(MICROBIT_PIN_COMPASS_DATA_READY),
    i2c(_i2c),
    accelerometer(&_accelerometer),
    storage(&_storage)
{
    init(id, address);
}

/**
  * Constructor.
  * Create a software representation of an e-compass.
  *
  * @param _i2c an instance of i2c, which the compass is accessible from.
  *
  * @param _accelerometer an instance of the accelerometer, used for tilt compensation.
  *
  * @param address the default address for the compass register on the i2c bus. Defaults to MAG3110_DEFAULT_ADDR.
  *
  * @param id the ID of the new MicroBitCompass object. Defaults to MAG3110_DEFAULT_ADDR.
  *
  * @code
  * MicroBitI2C i2c(I2C_SDA0, I2C_SCL0);
  *
  * MicroBitAccelerometer accelerometer(i2c);
  *
  * MicroBitCompass compass(i2c, accelerometer, storage);
  * @endcode
  */
MicroBitCompass::MicroBitCompass(MicroBitI2C& _i2c, MicroBitAccelerometer& _accelerometer, uint16_t address, uint16_t id) :
    average(),
    sample(),
    int1(MICROBIT_PIN_COMPASS_DATA_READY),
    i2c(_i2c),
    accelerometer(&_accelerometer),
    storage(NULL)
{
    init(id, address);
}

/**
  * Constructor.
  * Create a software representation of an e-compass.
  *
  * @param _i2c an instance of i2c, which the compass is accessible from.
  *
  * @param _storage an instance of MicroBitStorage, used to persist calibration data across resets.
  *
  * @param address the default address for the compass register on the i2c bus. Defaults to MAG3110_DEFAULT_ADDR.
  *
  * @param id the ID of the new MicroBitCompass object. Defaults to MAG3110_DEFAULT_ADDR.
  *
  * @code
  * MicroBitI2C i2c(I2C_SDA0, I2C_SCL0);
  *
  * MicroBitStorage storage;
  *
  * MicroBitCompass compass(i2c, storage);
  * @endcode
  */
MicroBitCompass::MicroBitCompass(MicroBitI2C& _i2c, MicroBitStorage& _storage, uint16_t address, uint16_t id) :
    average(),
    sample(),
    int1(MICROBIT_PIN_COMPASS_DATA_READY),
    i2c(_i2c),
    accelerometer(NULL),
    storage(&_storage)
{
    init(id, address);
}

/**
  * Constructor.
  * Create a software representation of an e-compass.
  *
  * @param _i2c an instance of i2c, which the compass is accessible from.
  *
  * @param address the default address for the compass register on the i2c bus. Defaults to MAG3110_DEFAULT_ADDR.
  *
  * @param id the ID of the new MicroBitCompass object. Defaults to MAG3110_DEFAULT_ADDR.
  *
  * @code
  * MicroBitI2C i2c(I2C_SDA0, I2C_SCL0);
  *
  * MicroBitCompass compass(i2c);
  * @endcode
  */
MicroBitCompass::MicroBitCompass(MicroBitI2C& _i2c, uint16_t address, uint16_t id) :
    average(),
    sample(),
    int1(MICROBIT_PIN_COMPASS_DATA_READY),
    i2c(_i2c),
    accelerometer(NULL),
    storage(NULL)
{
    init(id, address);
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
int MicroBitCompass::writeCommand(uint8_t reg, uint8_t value)
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
int MicroBitCompass::readCommand(uint8_t reg, uint8_t* buffer, int length)
{
    int result;

    if (buffer == NULL || length <= 0)
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
  * Issues a read of a given address, and returns the value.
  *
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the 16 bit register to access.
  *
  * @return The register value, interpreted as a 16 but signed value, or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
  */
int MicroBitCompass::read16(uint8_t reg)
{
    uint8_t cmd[2];
    int result;

    cmd[0] = reg;
    result = i2c.write(address, (const char *)cmd, 1);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    cmd[0] = 0x00;
    cmd[1] = 0x00;

    result = i2c.read(address, (char *)cmd, 2);
    if (result !=0)
        return MICROBIT_I2C_ERROR;

    return (int16_t) ((cmd[1] | (cmd[0] << 8))); //concatenate the MSB and LSB
}

/**
  * Issues a read of a given address, and returns the value.
  *
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the 16 bit register to access.
  *
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
  * Calculates a tilt compensated bearing of the device, using the accelerometer.
  */
int MicroBitCompass::tiltCompensatedBearing()
{
    // Precompute the tilt compensation parameters to improve readability.
    float phi = accelerometer->getRollRadians();
    float theta = accelerometer->getPitchRadians();

    float x = (float) getX(NORTH_EAST_DOWN);
    float y = (float) getY(NORTH_EAST_DOWN);
    float z = (float) getZ(NORTH_EAST_DOWN);

    // Precompute cos and sin of pitch and roll angles to make the calculation a little more efficient.
    float sinPhi = sin(phi);
    float cosPhi = cos(phi);
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);

    float bearing = (360*atan2(z*sinPhi - y*cosPhi, x*cosTheta + y*sinTheta*sinPhi + z*sinTheta*cosPhi)) / (2*PI);

    if (bearing < 0)
        bearing += 360.0;

    return (int) bearing;
}

/**
  * Calculates a non-tilt compensated bearing of the device.
  */
int MicroBitCompass::basicBearing()
{
    updateSample();

    float bearing = (atan2((double)(sample.y - average.y),(double)(sample.x - average.x)))*180/PI;

    if (bearing < 0)
        bearing += 360.0;

    return (int)(360.0 - bearing);
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
  * @return the current heading, in degrees. Or MICROBIT_CALIBRATION_IN_PROGRESS if the compass is calibrating.
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
  * Updates the local sample, only if the compass indicates that
  * data is stale.
  *
  * @note Can be used to trigger manual updates, if the device is running without a scheduler.
  *       Also called internally by all get[X,Y,Z]() member functions.
  */
int MicroBitCompass::updateSample()
{
    /**
      * Adds the compass to idle, if it hasn't been added already.
      * This is an optimisation so that the compass is only added on first 'use'.
      */
    if(!(status & MICROBIT_COMPASS_STATUS_ADDED_TO_IDLE))
    {
        fiber_add_idle_component(this);
        status |= MICROBIT_COMPASS_STATUS_ADDED_TO_IDLE;
    }

    // Poll interrupt line from compass (Active HI).
    // Interrupt is cleared on data read of MAG_OUT_X_MSB.
    if(int1)
    {
        sample.x = MAG3110_NORMALIZE_SAMPLE((int) read16(MAG_OUT_X_MSB));
        sample.y = MAG3110_NORMALIZE_SAMPLE((int) read16(MAG_OUT_Y_MSB));
        sample.z = MAG3110_NORMALIZE_SAMPLE((int) read16(MAG_OUT_Z_MSB));

        // Indicate that a new sample is available
        MicroBitEvent e(id, MICROBIT_COMPASS_EVT_DATA_UPDATE);
    }

    return MICROBIT_OK;
}

/**
  * Periodic callback from MicroBit idle thread.
  *
  * Calls updateSample().
  */
void MicroBitCompass::idleTick()
{
    updateSample();
}

/**
  * Reads the value of the X axis from the latest update retrieved from the magnetometer.
  *
  * @param system The coordinate system to use. By default, a simple cartesian system is provided.
  *
  * @return The magnetic force measured in the X axis, in nano teslas.
  *
  * @code
  * compass.getX();
  * @endcode
  */
int MicroBitCompass::getX(MicroBitCoordinateSystem system)
{
    updateSample();

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
  * Reads the value of the Y axis from the latest update retrieved from the magnetometer.
  *
  * @param system The coordinate system to use. By default, a simple cartesian system is provided.
  *
  * @return The magnetic force measured in the Y axis, in nano teslas.
  *
  * @code
  * compass.getY();
  * @endcode
  */
int MicroBitCompass::getY(MicroBitCoordinateSystem system)
{
    updateSample();

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
  * Reads the value of the Z axis from the latest update retrieved from the magnetometer.
  *
  * @param system The coordinate system to use. By default, a simple cartesian system is provided.
  *
  * @return The magnetic force measured in the Z axis, in nano teslas.
  *
  * @code
  * compass.getZ();
  * @endcode
  */
int MicroBitCompass::getZ(MicroBitCoordinateSystem system)
{
    updateSample();

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
    double x = getX();
    double y = getY();
    double z = getZ();

    return (int) sqrt(x*x + y*y + z*z);
}

/**
  * Configures the compass for the sample rate defined in this object.
  * The nearest values are chosen to those defined that are supported by the hardware.
  * The instance variables are then updated to reflect reality.
  *
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
		fiber_sleep(100);
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
  *
  * @param period the requested time between samples, in milliseconds.
  *
  * @return MICROBIT_OK or MICROBIT_I2C_ERROR if the magnetometer could not be updated.
  *
  * @code
  * // sample rate is now 20 ms.
  * compass.setPeriod(20);
  * @endcode
  *
  * @note The requested rate may not be possible on the hardware. In this case, the
  * nearest lower rate is chosen.
  */
int MicroBitCompass::setPeriod(int period)
{
    this->samplePeriod = period;
    return this->configure();
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
  * Attempts to read the 8 bit ID from the magnetometer, this can be used for
  * validation purposes.
  *
  * @return the 8 bit ID returned by the magnetometer, or MICROBIT_I2C_ERROR if the request fails.
  *
  * @code
  * compass.whoAmI();
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
  *
  * @return the temperature in degrees celsius, or MICROBIT_I2C_ERROR if the temperature reading could not be retreived
  *         from the accelerometer.
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

    updateSample();

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
  * @param calibration A CompassSample containing the offsets for the x, y and z axis.
  */
void MicroBitCompass::setCalibration(CompassSample calibration)
{
    if(this->storage != NULL)
        this->storage->put(ManagedString("compassCal"), (uint8_t *)&calibration, sizeof(CompassSample));

    average = calibration;
    status |= MICROBIT_COMPASS_STATUS_CALIBRATED;
}

/**
  * Provides the calibration data currently in use by the compass.
  *
  * More specifically, the x, y and z zero offsets of the compass.
  *
  * @return calibration A CompassSample containing the offsets for the x, y and z axis.
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
  * Destructor for MicroBitCompass, where we deregister this instance from the array of fiber components.
  */
MicroBitCompass::~MicroBitCompass()
{
    fiber_remove_idle_component(this);
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
