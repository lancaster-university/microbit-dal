//TODO changes
/*
The MIT License (MIT)

Copyright (c) 2019 ubirch GmbH.
Adapted from BMX055Accelerometer.cpp by (c) Lancaster University.

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
#include "BMX055Accelerometer.h"
#include "ErrorNo.h"
#include "MicroBitEvent.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"

// Set initial input parameters
// define X055 ACC full scale options
#define AFS_2G  0x03
#define AFS_4G  0x05
#define AFS_8G  0x08
#define AFS_16G 0x0C

enum ACCBW {    // define BMX055 accelerometer bandwidths
    ABW_8Hz,      // 7.81 Hz,  64 ms update time
    ABW_16Hz,     // 15.63 Hz, 32 ms update time
    ABW_31Hz,     // 31.25 Hz, 16 ms update time
    ABW_63Hz,     // 62.5  Hz,  8 ms update time
    ABW_125Hz,    // 125   Hz,  4 ms update time
    ABW_250Hz,    // 250   Hz,  2 ms update time
    ABW_500Hz,    // 500   Hz,  1 ms update time
    ABW_100Hz     // 1000  Hz,  0.5 ms update time
};

//
// Configuration table for available g force ranges.
// Maps g -> BMX055 full scale options
//
static const KeyValueTableEntry accelerometerRangeData[] = {
        {2,  AFS_2G},
        {4,  AFS_4G},
        {8,  AFS_8G},
        {16, AFS_16G}
};
CREATE_KEY_VALUE_TABLE(accelerometerRange, accelerometerRangeData);

//
// Configuration table for available data update frequency.
// maps microsecond period -> BMX055 bandwith selection parameter
//
static const KeyValueTableEntry accelerometerPeriodData[] = {
        {500,   ABW_100Hz},
        {1000,  ABW_500Hz},
        {2000,  ABW_250Hz},
        {4000,  ABW_125Hz},
        {8000,  ABW_63Hz},
        {16000, ABW_31Hz},
        {32000, ABW_16Hz},
        {64000, ABW_8Hz}
};
CREATE_KEY_VALUE_TABLE(accelerometerPeriod, accelerometerPeriodData);

#define I2C_CHECK(f) {if((f) != 0) return MICROBIT_I2C_ERROR;}

/**
 * Constructor.
 * Create a software abstraction of an accelerometer.
 *
 * @param coordinateSpace The orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
 * @param id The unique EventModel id of this component. Defaults to: MICROBIT_ID_ACCELEROMETER
 *
 */
BMX055Accelerometer::BMX055Accelerometer(MicroBitI2C &_i2c, MicroBitPin _int1, CoordinateSpace &coordinateSpace,
                                         uint16_t address, uint16_t id) : MicroBitAccelerometer(coordinateSpace, id),
                                                                          i2c(_i2c), int1(_int1) {
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
int BMX055Accelerometer::configure() {
    // First find the nearest sample rate to that specified.
    samplePeriod = accelerometerPeriod.getKey(samplePeriod * 1000);
    sampleRange = accelerometerRange.getKey(sampleRange);

    // Now configure the accelerometer accordingly.

    I2C_CHECK(i2c.writeRegister(address, BMX055_A_BGW_SOFTRESET, 0xB6)); // reset accelerometer
    fiber_sleep(100);// wait_ms(100);

    // configure accelerometer
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_PMU_RANGE, sampleRange & 0x0F)); // Set accelerometer full range
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_PMU_BW, samplePeriod & 0x0F));     // Set accelerometer bandwidth
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_D_HBW, 0x00));              // Use filtered data
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_BGW_SPI3_WDT, 0x06));       // Set watchdog timer for 50 ms

    // accelerometer
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_INT_EN_0,
                                0b00000000)); // Controls which interrupt engines in group 0 are enabled.
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_INT_EN_1,
                                0b00010000)); // Controls which interrupt engines in group 1 are enabled.
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_INT_EN_2,
                                0b00000000)); // Controls which interrupt engines in group 2 are enabled.
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_INT_MAP_0,
                                0b00000000)); // Controls which interrupt signals are mapped to the INT1 pin.
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_INT_MAP_1,
                                0b10000001)); // Controls which interrupt signals are mapped to the INT1 and INT2 pins.
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_INT_MAP_2,
                                0b00000000)); // Controls which interrupt signals are mapped to the INT2 pin.
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_INT_SRC,
                                0b00000000)); // Contains the data source definition for interrupts with selectable data source.
    I2C_CHECK(i2c.writeRegister(address, BMX055_A_INT_OUT_CTRL,
                                0b00000010)); // Contains the behavioural configuration (electrical behavior) of the interrupt pins.

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
int BMX055Accelerometer::requestUpdate() {
    // Ensure we're scheduled to update the data periodically

    if (!(status & MICROBIT_ACCEL_ADDED_TO_IDLE)) {
        fiber_add_idle_component(this);
        status |= MICROBIT_ACCEL_ADDED_TO_IDLE;
    }

    int dataReady = !int1.getDigitalValue();
    // Poll interrupt line from device (ACTIVE LO)
    if (dataReady) {
        uint8_t data[6];
        int result;
        int16_t *x;
        int16_t *y;
        int16_t *z;


        // Read the combined accelerometer and magnetometer data.
        result = i2c.readRegister(address, BMX055_A_D_X_LSB, data, 6);

        if (result != 0)
            return MICROBIT_I2C_ERROR;

        // Read in each reading as a 16 bit little endian value, and scale to 10 bits.
        x = ((int16_t *) &data[0]);
        y = ((int16_t *) &data[2]);
        z = ((int16_t *) &data[4]);

        *x = *x / 32;
        *y = *y / 32;
        *z = *z / 32;

        // Scale into millig (approx) and align to ENU coordinate system
        sampleENU.x = ((int) (*x)) * sampleRange;
        sampleENU.y = ((int) (*y)) * sampleRange;
        sampleENU.z = ((int) (*z)) * sampleRange;

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
void BMX055Accelerometer::idleTick() {
    requestUpdate();
}

/**
 * Attempts to read the 8 bit WHO_AM_I value from the accelerometer
 *
 * @return true if the WHO_AM_I value is succesfully read. false otherwise.
 */
int BMX055Accelerometer::isDetected(MicroBitI2C &i2c, uint16_t address) {
    i2c.writeRegister(address, BMX055_A_BGW_SOFTRESET, 0xB6);
    fiber_sleep(100);// wait_ms(100);
    return i2c.readRegister(address, BMX055_A_WHOAMI) == BMX055_A_WHOAMI_VAL;
}

/**
 * Destructor.
 */
BMX055Accelerometer::~BMX055Accelerometer() {
}

