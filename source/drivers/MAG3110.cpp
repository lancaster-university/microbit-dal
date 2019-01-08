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

/**
 * Class definition for a MAG3110 3 axis magnetometer.
 *
 * Represents an implementation of the Freescale MAG3110 3 axis magnetometer
 */
#include "MAG3110.h"
#include "ErrorNo.h"
#include "MicroBitEvent.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"

//
// Configuration table for available data update frequency.
// maps microsecond period -> CTRL_REG1 data rate selection bits [3..5]
//
static const KeyValueTableEntry magnetometerPeriodData[] = {
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
CREATE_KEY_VALUE_TABLE(magnetometerPeriod, magnetometerPeriodData);


/**
 * Configures the compass for the sample rate defined in this object. 
 * The nearest values are chosen to those defined
 * that are supported by the hardware. The instance variables are then
 * updated to reflect reality.
 *
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the compass could not be configured.
 */
int MAG3110::configure()
{
    int result;
    uint8_t value;

    // First, take the device offline, so it can be configured.
    result = i2c.writeRegister(address, MAG_CTRL_REG1, 0x00);
    if (result != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;

    // Wait for the part to enter standby mode...
    while(1)
    {
        // Read the status of the part...
        // If we can't communicate with it over I2C, pass on the error.
        result = i2c.readRegister(address,MAG_SYSMOD, &value, 1);
        if (result == MICROBIT_I2C_ERROR)
            return MICROBIT_I2C_ERROR;

        // if the part in in standby, we're good to carry on.
        if((value & 0x03) == 0)
            break;

        // Perform a power efficient sleep...
		fiber_sleep(100);
    }

    // First find the nearest sample rate to that specified.
    samplePeriod = magnetometerPeriod.getKey(samplePeriod * 1000) / 1000;

    // Now configure the magnetometer accordingly.
    // Enable automatic reset after each sample;
    result = i2c.writeRegister(address, MAG_CTRL_REG2, 0xA0);
    if (result != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;


    // Bring the device online, with the requested sample frequency.
    result = i2c.writeRegister(address, MAG_CTRL_REG1, magnetometerPeriod.get(samplePeriod * 1000) | 0x01);
    if (result != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;

    return MICROBIT_OK;
}

/**
  * Constructor.
  * Create a software abstraction of an FXSO8700 combined magnetometer/magnetometer
  *
  * @param _i2c an instance of I2C used to communicate with the device.
  *
  * @param address the default I2C address of the magnetometer. Defaults to: FXS8700_DEFAULT_ADDR.
  *
 */
MAG3110::MAG3110(MicroBitI2C &_i2c, MicroBitPin _int1, CoordinateSpace &coordinateSpace, uint16_t address, uint16_t id) : MicroBitCompass(coordinateSpace, id), i2c(_i2c), int1(_int1)
{
    // Store our identifiers.
    this->address = address;

    // Configure and enable the magnetometer.
    configure();
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
int MAG3110::requestUpdate()
{
    // Ensure we're scheduled to update the data periodically
    if(!(status & MICROBIT_COMPASS_STATUS_ADDED_TO_IDLE))
    {
        fiber_add_idle_component(this);
        status |= MICROBIT_COMPASS_STATUS_ADDED_TO_IDLE;
    }

    // Poll interrupt line from device (ACTIVE HI)
    if(int1.getDigitalValue())
    {
        uint8_t data[6];
        int16_t s;
        uint8_t *lsb = (uint8_t *) &s;
        uint8_t *msb = lsb + 1;
        int result;

        // Read the combined magnetometer and magnetometer data.
        result = i2c.readRegister(address, MAG_OUT_X_MSB, data, 6);

        if (result !=0)
            return MICROBIT_I2C_ERROR;
        // Scale the 14 bit data (packed into 16 bits) into SI units (milli-g) and translate into signed little endian, and align to ENU coordinate system
        *msb = data[0];
        *lsb = data[1];
        sampleENU.y = MAG3110_NORMALIZE_SAMPLE(s); 

        *msb = data[2];
        *lsb = data[3];
        sampleENU.x = -MAG3110_NORMALIZE_SAMPLE(s); 

        *msb = data[4];
        *lsb = data[5];
        sampleENU.z = -MAG3110_NORMALIZE_SAMPLE(s); 

        // Inform the higher level driver that raw data has been updated.
        update();
    }

    return MICROBIT_OK;
}


/**
  * A periodic callback invoked by the fiber scheduler idle thread.
  *
  * Internally calls updateSample().
  */
void MAG3110::idleTick()
{
    requestUpdate();
}

/**
 * Attempts to read the 8 bit WHO_AM_I value from the accelerometer
 *
 * @return true if the WHO_AM_I value is succesfully read. false otherwise.
 */
int MAG3110::isDetected(MicroBitI2C &i2c, uint16_t address)
{
    return i2c.readRegister(address, MAG_WHOAMI) == MAG3110_WHOAMI_VAL;
}

/**
  * Destructor for FXS8700, where we deregister from the array of fiber components.
  */
MAG3110::~MAG3110()
{
}

