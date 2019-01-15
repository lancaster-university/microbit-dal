/*
The MIT License (MIT)

Copyright (c) 2019 ubirch GmbH.
Adapted from LSM303Accelerometer.h by (c) Lancaster University.

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

#ifndef BMX055_ACCELEROMETER_H
#define BMX055_ACCELEROMETER_H

#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "CoordinateSystem.h"
#include "MicroBitAccelerometer.h"
#include "MicroBitI2C.h"
#include "MicroBitUtil.h"

#define BMX055_A_DEFAULT_ADDR   0x30 //!< address of the BMX055 accelerometer

#define BMX055_A_WHOAMI        0x00   // should return 0xFA
//#define BMX055_A_Reserved    0x01
#define BMX055_A_D_X_LSB       0x02
#define BMX055_A_D_X_MSB       0x03
#define BMX055_A_D_Y_LSB       0x04
#define BMX055_A_D_Y_MSB       0x05
#define BMX055_A_D_Z_LSB       0x06
#define BMX055_A_D_Z_MSB       0x07
#define BMX055_A_D_TEMP        0x08
#define BMX055_A_INT_STATUS_0  0x09
#define BMX055_A_INT_STATUS_1  0x0A
#define BMX055_A_INT_STATUS_2  0x0B
#define BMX055_A_INT_STATUS_3  0x0C
//#define BMX055_A_Reserved    0x0D
#define BMX055_A_FIFO_STATUS   0x0E
#define BMX055_A_PMU_RANGE     0x0F
#define BMX055_A_PMU_BW        0x10
#define BMX055_A_PMU_LPW       0x11
#define BMX055_A_PMU_LOW_POWER 0x12
#define BMX055_A_D_HBW         0x13
#define BMX055_A_BGW_SOFTRESET 0x14
//#define BMX055_A_Reserved    0x15
#define BMX055_A_INT_EN_0      0x16
#define BMX055_A_INT_EN_1      0x17
#define BMX055_A_INT_EN_2      0x18
#define BMX055_A_INT_MAP_0     0x19
#define BMX055_A_INT_MAP_1     0x1A
#define BMX055_A_INT_MAP_2     0x1B
//#define BMX055_A_Reserved    0x1C
//#define BMX055_A_Reserved    0x1D
#define BMX055_A_INT_SRC       0x1E
//#define BMX055_A_Reserved    0x1F
#define BMX055_A_INT_OUT_CTRL  0x20
#define BMX055_A_INT_RST_LATCH 0x21
#define BMX055_A_INT_0         0x22
#define BMX055_A_INT_1         0x23
#define BMX055_A_INT_2         0x24
#define BMX055_A_INT_3         0x25
#define BMX055_A_INT_4         0x26
#define BMX055_A_INT_5         0x27
#define BMX055_A_INT_6         0x28
#define BMX055_A_INT_7         0x29
#define BMX055_A_INT_8         0x2A
#define BMX055_A_INT_9         0x2B
#define BMX055_A_INT_A         0x2C
#define BMX055_A_INT_B         0x2D
#define BMX055_A_INT_C         0x2E
#define BMX055_A_INT_D         0x2F
#define BMX055_A_FIFO_CONFIG_0 0x30
//#define BMX055_A_Reserved    0x31
#define BMX055_A_PMU_SELF_TEST 0x32
#define BMX055_A_TRIM_NVM_CTRL 0x33
#define BMX055_A_BGW_SPI3_WDT  0x34
//#define BMX055_A_Reserved    0x35
#define BMX055_A_OFC_CTRL      0x36
#define BMX055_A_OFC_SETTING   0x37
#define BMX055_A_OFC_OFFSET_X  0x38
#define BMX055_A_OFC_OFFSET_Y  0x39
#define BMX055_A_OFC_OFFSET_Z  0x3A
#define BMX055_A_TRIM_GPO      0x3B
#define BMX055_A_TRIM_GP1      0x3C
//#define BMX055_A_Reserved    0x3D
#define BMX055_A_FIFO_CONFIG_1 0x3E
#define BMX055_A_FIFO_DATA     0x3F

#define BMX055_A_WHOAMI_VAL     0xFA

/**
 * Class definition for BMX055Accelerometer.
 *
 * This class defines the interface for the BMX055 accelerometer.
 */
class BMX055Accelerometer : public MicroBitAccelerometer
{
    MicroBitI2C&            i2c;                    // The I2C interface to use.
    MicroBitPin             int1;                   // Data ready interrupt.
    uint16_t                address;                // I2C address of this accelerometer.

    public:

    /**
     * Constructor.
     * Create a software abstraction of an accelerometer.
     *
     * @param coordinateSpace The orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
     * @param id The unique EventModel id of this component. Defaults to: MICROBIT_ID_ACCELEROMETER
     *
     */
    BMX055Accelerometer(MicroBitI2C& _i2c, MicroBitPin _int1, CoordinateSpace &coordinateSpace, uint16_t address = BMX055_A_DEFAULT_ADDR, uint16_t id = MICROBIT_ID_ACCELEROMETER);

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
    virtual int configure();

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
    static int isDetected(MicroBitI2C &i2c, uint16_t address = BMX055_A_DEFAULT_ADDR);

    /**
     * Destructor.
     */
    ~BMX055Accelerometer();

};

#endif

