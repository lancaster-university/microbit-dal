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
 * Class definition for an MMA8653 3 axis accelerometer.
 *
 * Represents an implementation of the MMA8653 3 axis accelerometer
 * Also includes basic data caching and on demand activation.
 */
#include "MicroBitConfig.h"
#include "MMA8653.h"
#include "ErrorNo.h"
#include "MicroBitEvent.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"

//
// Configuration table for available g force ranges.
// Maps g -> XYZ_DATA_CFG bit [0..1]
//
static const KeyValueTableEntry accelerometerRangeData[] = {
    {2, 0},
    {4, 1},
    {8, 2}
};
CREATE_KEY_VALUE_TABLE(accelerometerRange, accelerometerRangeData);

//
// Configuration table for available data update frequency.
// maps microsecond period -> CTRL_REG1 data rate selection bits [3..5]
//
static const KeyValueTableEntry accelerometerPeriodData[] = {
    {1250,      0x00},
    {2500,      0x08},
    {5000,      0x10},
    {10000,     0x18},
    {20000,     0x20},
    {80000,     0x28},
    {160000,    0x30},
    {640000,    0x38}
};
CREATE_KEY_VALUE_TABLE(accelerometerPeriod, accelerometerPeriodData);


/**
 * Constructor.
 * Create a software abstraction of an accelerometer.
 *
 * @param coordinateSpace The orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
 * @param id The unique EventModel id of this component. Defaults to: MICROBIT_ID_ACCELEROMETER
 *
 */
MMA8653::MMA8653(MicroBitI2C& _i2c, MicroBitPin _int1, CoordinateSpace &coordinateSpace, uint16_t address, uint16_t id) : MicroBitAccelerometer(coordinateSpace, id), i2c(_i2c), int1(_int1)
{
    // Store our identifiers.
    this->status = 0;
    this->address = address;

    // Configure and enable the accelerometer.
    configure();
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
int MMA8653::configure()
{
    int result;
    uint8_t value;

    // First find the nearest sample rate to that specified.
    samplePeriod = accelerometerPeriod.getKey(samplePeriod * 1000) / 1000;
    sampleRange = accelerometerRange.getKey(sampleRange);

    // Now configure the accelerometer accordingly.

    // First place the device into standby mode, so it can be configured.
    result = i2c.writeRegister(address, MMA8653_CTRL_REG1, 0x00);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Enable high precisiosn mode. This consumes a bit more power, but still only 184 uA!
    result = i2c.writeRegister(address, MMA8653_CTRL_REG2, 0x10);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Enable the INT1 interrupt pin.
    result = i2c.writeRegister(address, MMA8653_CTRL_REG4, 0x01);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Select the DATA_READY event source to be routed to INT1
    result = i2c.writeRegister(address, MMA8653_CTRL_REG5, 0x01);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Configure for the selected g range.
    value = accelerometerRange.get(sampleRange);
    result = i2c.writeRegister(address, MMA8653_XYZ_DATA_CFG, value);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    // Bring the device back online, with 10bit wide samples at the requested frequency.
    value = accelerometerPeriod.get(samplePeriod * 1000);
    result = i2c.writeRegister(address, MMA8653_CTRL_REG1, value | 0x01);
    if (result != 0)
        return MICROBIT_I2C_ERROR;

    return MICROBIT_OK;
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
int MMA8653::requestUpdate()
{
    // Ensure we're scheduled to update the data periodically
    if(!(status & MICROBIT_ACCEL_ADDED_TO_IDLE))
    {
        fiber_add_idle_component(this);
        status |= MICROBIT_ACCEL_ADDED_TO_IDLE;
    }

    // Poll interrupt line from device (ACTIVE LO)
    if(!int1.getDigitalValue())
    {
        int8_t data[6];
        int result;
        Sample3D s;

        // Read the combined accelerometer and magnetometer data.
        result = i2c.readRegister(address, MMA8653_OUT_X_MSB, (uint8_t *)data, 6);

        if (result !=0)
            return MICROBIT_I2C_ERROR;

        // read MSB values and normalize the data in the -1024...1024 range
        s.x = 8 * data[0];
        s.y = 8 * data[2];
        s.z = 8 * data[4];

#if CONFIG_ENABLED(USE_ACCEL_LSB)
        // Add in LSB values.
        s.x += (data[1] / 64);
        s.y += (data[3] / 64);
        s.z += (data[5] / 64);
#endif

        // Scale into millig (approx) and align to ENU coordinate system
        sampleENU.x = -s.y * sampleRange;
        sampleENU.y = s.x * sampleRange;
        sampleENU.z = s.z * sampleRange;

        // indicate that new data is available.
        update();
    }

    return MICROBIT_OK;
}

/**
  * A periodic callback invoked by the fiber scheduler idle thread.
  *
  * Internally calls updateSample().
  */
void MMA8653::idleTick()
{
    requestUpdate();
}

/**
 * Attempts to read the 8 bit WHO_AM_I value from the accelerometer
 *
 * @return true if the WHO_AM_I value is succesfully read. false otherwise.
 */
int MMA8653::isDetected(MicroBitI2C &i2c, uint16_t address)
{
    return i2c.readRegister(address, MMA8653_WHOAMI) == MMA8653_WHOAMI_VAL;
}

/**
 * Destructor.
 */
MMA8653::~MMA8653()
{
}

