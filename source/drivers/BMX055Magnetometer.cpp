//todo changes
/*
The MIT License (MIT)

Copyright (c) 2019 Calliope gGmbH.

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

#include "BMX055Magnetometer.h"
#include "BMX055Accelerometer.h"
#include "MicroBitEvent.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"


//enum Gscale {
//    GFS_2000DPS = 0,
//    GFS_1000DPS,
//    GFS_500DPS,
//    GFS_250DPS,
//    GFS_125DPS
//};
//
//enum GODRBW {
//    G_2000Hz523Hz = 0, // 2000 Hz ODR and unfiltered (bandwidth 523Hz)
//    G_2000Hz230Hz,
//    G_1000Hz116Hz,
//    G_400Hz47Hz,
//    G_200Hz23Hz,
//    G_100Hz12Hz,
//    G_200Hz64Hz,
//    G_100Hz32Hz  // 100 Hz ODR and 32 Hz bandwidth
//};

enum MODR {
    MODR_10Hz = 0,   // 10 Hz ODR
    MODR_2Hz,   // 2 Hz ODR
    MODR_6Hz,   // 6 Hz ODR
    MODR_8Hz,   // 8 Hz ODR
    MODR_15Hz,   // 15 Hz ODR
    MODR_20Hz,   // 20 Hz ODR
    MODR_25Hz,   // 25 Hz ODR
    MODR_30Hz        // 30 Hz ODR
};


//
// Configuration table for available data update frequency.
// maps microsecond period -> BMX055_CFG_REG_A_M data rate selection bits [2..3]
//
static const KeyValueTableEntry magnetometerPeriodData[] = {
        {33333,  MODR_30Hz},
        {40000,  MODR_25Hz},
        {50000,  MODR_20Hz},
        {66666,  MODR_15Hz},
        {100000, MODR_10Hz},
        {125000, MODR_8Hz},
        {166666, MODR_6Hz},
        {500000, MODR_2Hz}
};
CREATE_KEY_VALUE_TABLE(magnetometerPeriod, magnetometerPeriodData);

#define I2C_CHECK(f) {if((f) != 0) return MICROBIT_I2C_ERROR;}

/**
 * Configures the compass for the sample rate defined in this object. 
 * The nearest values are chosen to those defined
 * that are supported by the hardware. The instance variables are then
 * updated to reflect reality.
 *
 * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the compass could not be configured.
 */
int BMX055Magnetometer::configure() {
    // First find the nearest sample rate to that specified.
    samplePeriod = magnetometerPeriod.getKey(samplePeriod * 1000);

    // Softreset magnetometer, ends up in sleep mode, so wake up afterwards
    I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_PWR_CNTL1, 0x82));
    fiber_sleep(100);//wait_ms(100);
    I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_PWR_CNTL1, 0x01));
    fiber_sleep(100);//wait_ms(100);

    I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_PWR_CNTL2, samplePeriod << 3));
    I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_INT_EN_2,
                                0b10000100)); // Enable data ready pin interrupt, active high

    // Set up four standard configurations for the magnetometer
    switch (Mmode) {
        case lowPower:
            // Low-power
        I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_REP_XY, 0x01));  // 3 repetitions (oversampling)
            I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_REP_Z, 0x02));  // 3 repetitions (oversampling)
            break;
        case enhancedRegular:
            // Enhanced Regular
        I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_REP_XY, 0x07));  // 15 repetitions (oversampling)
            I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_REP_Z, 0x22));  // 27 repetitions (oversampling)
            break;
        case highAccuracy:
            // High Accuracy
        I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_REP_XY, 0x17));  // 47 repetitions (oversampling)
            I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_REP_Z, 0x51));  // 83 repetitions (oversampling)
            break;
        case regular:
            // Regular
        I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_REP_XY, 0x04));  //  9 repetitions (oversampling)
            I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_REP_Z, 0x16));  // 15 repetitions (oversampling)
            break;
        default:
            return MICROBIT_INVALID_PARAMETER;
    }

    // read trim data
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_X1, (uint8_t *) &dig_x1, 1));
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_X2, (uint8_t *) &dig_x2, 1));
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_Y1, (uint8_t *) &dig_y1, 1));
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_Y2, (uint8_t *) &dig_y2, 1));
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_XY1, (uint8_t *) &dig_xy1, 1));
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_XY2, (uint8_t *) &dig_xy2, 1));

    uint8_t tmp[2];
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_Z1_LSB, tmp, 2));
    dig_z1 = (uint16_t) (((uint16_t) tmp[1] << 8) | tmp[0]);
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_Z2_LSB, tmp, 2));
    dig_z2 = (int16_t) (((int16_t) tmp[1] << 8) | tmp[0]);
    dig_z3 = (int16_t) (((int16_t) tmp[1] << 8) | tmp[0]);
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_Z4_LSB, tmp, 2));
    dig_z4 = (int16_t) (((int16_t) tmp[1] << 8) | tmp[0]);
    I2C_CHECK(i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMM050_DIG_XYZ1_LSB, tmp, 2));
    dig_xyz1 = (uint16_t) (((uint16_t) tmp[1] << 8) | tmp[0]);

//    uint8_t offset[3];
//    I2C_CHECK(i2c.readRegister(BMX055_A_DEFAULT_ADDR, BMX055_A_OFC_OFFSET_X, &offset[0], 1));
//    I2C_CHECK(i2c.readRegister(BMX055_A_DEFAULT_ADDR, BMX055_A_OFC_OFFSET_Y, &offset[1], 1));
//    I2C_CHECK(i2c.readRegister(BMX055_A_DEFAULT_ADDR, BMX055_A_OFC_OFFSET_Z, &offset[2], 1));
//    if (offset[0] || offset[1] || offset[2]) {
    // disable microbit calibration if the accelerometer is already calibrated
//        setCalibration(CompassCalibration());
//    }

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
BMX055Magnetometer::BMX055Magnetometer(MicroBitI2C &_i2c, MicroBitPin _int1, CoordinateSpace &coordinateSpace,
                                       uint16_t address, uint16_t id) : MicroBitCompass(coordinateSpace, id), i2c(_i2c),
                                                                        int1(_int1) {
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
int BMX055Magnetometer::requestUpdate() {
    // Ensure we're scheduled to update the data periodically
    if (!(status & MICROBIT_COMPASS_STATUS_ADDED_TO_IDLE)) {
        fiber_add_idle_component(this);
        status |= MICROBIT_COMPASS_STATUS_ADDED_TO_IDLE;
    }

    uint8_t rawData[8];  // x/y/z hall magnetic field data, and Hall resistance data

    if (i2c.readRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_XOUT_LSB, rawData, 8) != MICROBIT_OK)
        return MICROBIT_I2C_ERROR;

    if (rawData[6] & 0x01) { // Check if data ready status bit is set
        int16_t x, y, z;
        int16_t mdata_x = 0, mdata_y = 0, mdata_z = 0, temp = 0;
        uint16_t data_r = 0;

        mdata_x = (int16_t) (((int16_t) rawData[1] << 8) | rawData[0]) >> 3;  // 13-bit
        mdata_y = (int16_t) (((int16_t) rawData[3] << 8) | rawData[2]) >> 3;  // 13-bit
        mdata_z = (int16_t) (((int16_t) rawData[5] << 8) | rawData[4]) >> 1;  // 15-bit
        data_r = (uint16_t) (((uint16_t) rawData[7] << 8) | rawData[6]) >> 2;  // 14-bit (hall resistance)

        // calculate temperature compensated 16-bit magnetic fields
        temp = ((int16_t) (((uint16_t) ((((int32_t) dig_xyz1) << 14) / (data_r != 0 ? data_r : dig_xyz1))) -
                ((uint16_t) 0x4000)));
        x = ((int16_t) ((((int32_t) mdata_x) *
                ((((((((int32_t) dig_xy2) * ((((int32_t) temp) * ((int32_t) temp)) >> 7)) +
                        (((int32_t) temp) * ((int32_t) (((int16_t) dig_xy1) << 7)))) >> 9) +
                   ((int32_t) 0x100000)) * ((int32_t) (((int16_t) dig_x2) + ((int16_t) 0xA0)))) >> 12))
                >> 13)) +
            (((int16_t) dig_x1) << 3);

        temp = ((int16_t) (((uint16_t) ((((int32_t) dig_xyz1) << 14) / (data_r != 0 ? data_r : dig_xyz1))) -
                ((uint16_t) 0x4000)));
        y = ((int16_t) ((((int32_t) mdata_y) *
                ((((((((int32_t) dig_xy2) * ((((int32_t) temp) * ((int32_t) temp)) >> 7)) +
                        (((int32_t) temp) * ((int32_t) (((int16_t) dig_xy1) << 7)))) >> 9) +
                   ((int32_t) 0x100000)) * ((int32_t) (((int16_t) dig_y2) + ((int16_t) 0xA0)))) >> 12))
                >> 13)) +
            (((int16_t) dig_y1) << 3);
        z = (((((int32_t) (mdata_z - dig_z4)) << 15) - ((((int32_t) dig_z3) * ((int32_t) (((int16_t) data_r) -
                ((int16_t) dig_xyz1))))
                >> 2)) /
             (dig_z2 + ((int16_t) (((((int32_t) dig_z1) * ((((int16_t) data_r) << 1))) + (1 << 15)) >> 16))));

        // Align to ENU coordinate system
        sampleENU.x = -BMX055_M_NORMALIZE_SAMPLE(y);
        sampleENU.y = BMX055_M_NORMALIZE_SAMPLE(x);
        sampleENU.z = BMX055_M_NORMALIZE_SAMPLE(z);

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
void BMX055Magnetometer::idleTick() {
    requestUpdate();
}

/**
 * Attempts to read the 8 bit WHO_AM_I value from the accelerometer
 *
 * @return true if the WHO_AM_I value is succesfully read. false otherwise.
 */
int BMX055Magnetometer::isDetected(MicroBitI2C &i2c, uint16_t address) {
    I2C_CHECK(i2c.writeRegister(BMX055_M_DEFAULT_ADDR, BMX055_M_PWR_CNTL1, 0x01));
    fiber_sleep(100);// wait_ms(100);
    return i2c.readRegister(address, BMX055_M_WHOAMI) == BMX055_M_WHOAMI_VAL;
}

/**
  * Destructor for FXS8700, where we deregister from the array of fiber components.
  */
BMX055Magnetometer::~BMX055Magnetometer() {
}

