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

#ifndef BMX055_M_H
#define BMX055_M_H

#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "CoordinateSystem.h"
#include "MicroBitCompass.h"
#include "MicroBitI2C.h"
#include "MicroBitUtil.h"

/**
  * Term to convert sample data into SI units.
  */
#define BMX055_M_NORMALIZE_SAMPLE(x) (100 * (int)(x))

#define BMX055_M_DEFAULT_ADDR   0x20 //!< address of the BMX055 magnetometer

// BMX055 magnetometer registers
#define BMX055_M_WHOAMI         0x40  // should return 0x32
#define BMX055_M_Reserved       0x41
#define BMX055_M_XOUT_LSB       0x42
#define BMX055_M_XOUT_MSB       0x43
#define BMX055_M_YOUT_LSB       0x44
#define BMX055_M_YOUT_MSB       0x45
#define BMX055_M_ZOUT_LSB       0x46
#define BMX055_M_ZOUT_MSB       0x47
#define BMX055_M_ROUT_LSB       0x48
#define BMX055_M_ROUT_MSB       0x49
#define BMX055_M_INT_STATUS     0x4A
#define BMX055_M_PWR_CNTL1      0x4B
#define BMX055_M_PWR_CNTL2      0x4C
#define BMX055_M_INT_EN_1       0x4D
#define BMX055_M_INT_EN_2       0x4E
#define BMX055_M_LOW_THS        0x4F
#define BMX055_M_HIGH_THS       0x50
#define BMX055_M_REP_XY         0x51
#define BMX055_M_REP_Z          0x52

/* Trim Extended Registers */
#define BMM050_DIG_X1             0x5D // needed for magnetic field calculation
#define BMM050_DIG_Y1             0x5E
#define BMM050_DIG_Z4_LSB         0x62
#define BMM050_DIG_Z4_MSB         0x63
#define BMM050_DIG_X2             0x64
#define BMM050_DIG_Y2             0x65
#define BMM050_DIG_Z2_LSB         0x68
#define BMM050_DIG_Z2_MSB         0x69
#define BMM050_DIG_Z1_LSB         0x6A
#define BMM050_DIG_Z1_MSB         0x6B
#define BMM050_DIG_XYZ1_LSB       0x6C
#define BMM050_DIG_XYZ1_MSB       0x6D
#define BMM050_DIG_Z3_LSB         0x6E
#define BMM050_DIG_Z3_MSB         0x6F
#define BMM050_DIG_XY2            0x70
#define BMM050_DIG_XY1            0x71

#define BMX055_M_WHOAMI_VAL   0x32

enum BMX055_M_MODE {
    lowPower         = 0,   // rms noise ~1.0 microTesla, 0.17 mA power
    regular             ,   // rms noise ~0.6 microTesla, 0.5 mA power
    enhancedRegular     ,   // rms noise ~0.5 microTesla, 0.8 mA power
    highAccuracy            // rms noise ~0.3 microTesla, 4.9 mA power
};


/**
 * Class definition for BMX055 magnetometer.
 *
 * This class provides the low level driver implementation for the BMX05 Magnetometer
 *
 */
class BMX055Magnetometer : public MicroBitCompass
{
    MicroBitI2C&            i2c;                    // The I2C interface to use.
    MicroBitPin             int1;                   // Data ready interrupt.
    uint16_t                address;                // I2C address of this compass.
    uint8_t                 Mmode  = regular;       // Choose magnetometer operation mode

public:

    /**
     * Constructor.
     * Create a software abstraction of an compass.
     *
     * @param coordinateSpace The orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
     * @param id The unique EventModel id of this component. Defaults to: MICROBIT_ID_ACCELEROMETER
     *
     */
    BMX055Magnetometer(MicroBitI2C& _i2c, MicroBitPin _int1, CoordinateSpace &coordinateSpace, uint16_t address = BMX055_M_DEFAULT_ADDR, uint16_t id = MICROBIT_ID_COMPASS);

    /**
     * Configures the compass for the sample rate defined in this object. 
     * The nearest values are chosen to those defined
     * that are supported by the hardware. The instance variables are then
     * updated to reflect reality.
     *
     * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the compass could not be configured.
     */
    virtual int configure();

    /**
     * Poll to see if new data is available from the hardware. If so, update it.
     * n.b. it is not necessary to explicitly call this function to update data
     * (it normally happens in the background when the scheduler is idle), but a check is performed
     * if the user explicitly requests up to date data.
     *
     * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the update fails.
     */
    virtual int requestUpdate();

    /**
     * A periodic callback invoked by the fiber scheduler idle thread.
     *
     * Internally calls updateSample().
     */
    virtual void idleTick();

    /**
     * Attempts to read the 8 bit WHO_AM_I value from the accelerometer
     *
     * @return true if the WHO_AM_I value is succesfully read. false otherwise.
     */
    static int isDetected(MicroBitI2C &i2c, uint16_t address = BMX055_M_DEFAULT_ADDR);

    /**
     * Destructor.
     */
    ~BMX055Magnetometer();


private:
    signed char   dig_x1;
    signed char   dig_y1;
    signed char   dig_x2;
    signed char   dig_y2;
    uint16_t      dig_z1;
    int16_t       dig_z2;
    int16_t       dig_z3;
    int16_t       dig_z4;
    unsigned char dig_xy1;
    signed char   dig_xy2;
    uint16_t      dig_xyz1;
};

#endif

