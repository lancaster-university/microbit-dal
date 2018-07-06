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

#ifndef LSM303_A_H
#define LSM303_A_H

#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "CoordinateSystem.h"
#include "MicroBitAccelerometer.h"
#include "MicroBitI2C.h"
#include "MicroBitUtil.h"

/**
  * I2C constants
  */
#define LSM303_A_DEFAULT_ADDR           0x32

/**
  * LSM303 Register map (partial)
  */

#define LSM303_STATUS_REG_AUX_A		    0x07
#define LSM303_OUT_TEMP_L_A		        0x0C
#define LSM303_OUT_TEMP_H_A		        0x0D
#define LSM303_INT_COUNTER_REG_A		0x0E
#define LSM303_WHO_AM_I_A		        0x0F
#define LSM303_TEMP_CFG_REG_A		    0x1F
#define LSM303_CTRL_REG1_A		        0x20
#define LSM303_CTRL_REG2_A		        0x21
#define LSM303_CTRL_REG3_A		        0x22
#define LSM303_CTRL_REG4_A		        0x23
#define LSM303_CTRL_REG5_A		        0x24
#define LSM303_CTRL_REG6_A		        0x25
#define LSM303_DATACAPTURE_A		    0x26
#define LSM303_STATUS_REG_A		        0x27
#define LSM303_OUT_X_L_A		        0x28
#define LSM303_OUT_X_H_A		        0x29
#define LSM303_OUT_Y_L_A		        0x2A
#define LSM303_OUT_Y_H_A		        0x2B
#define LSM303_OUT_Z_L_A		        0x2C
#define LSM303_OUT_Z_H_A		        0x2D
#define LSM303_FIFO_CTRL_REG_A		    0x2E
#define LSM303_FIFO_SRC_REG_A		    0x2F
#define LSM303_INT1_CFG_A		        0x30
#define LSM303_INT1_SRC_A		        0x31
#define LSM303_INT1_THS_A		        0x32
#define LSM303_INT1_DURATION_A		    0x33
#define LSM303_INT2_CFG_A		        0x34
#define LSM303_INT2_SRC_A		        0x35
#define LSM303_INT2_THS_A		        0x36
#define LSM303_INT2_DURATION_A		    0x37
#define LSM303_CLICK_CFG_A		        0x38
#define LSM303_CLICK_SRC_A		        0x39
#define LSM303_CLICK_THS_A		        0x3A
#define LSM303_TIME_LIMIT_A		        0x3B
#define LSM303_TIME_LATENCY_A		    0x3C
#define LSM303_TIME_WINDOW_A		    0x3D
#define LSM303_ACT_THS_A		        0x3E
#define LSM303_ACT_DUR_A		        0x3F

/**
  * LSM303_A constants
  */
#define LSM303_A_WHOAMI_VAL             0x33

/**
 * Class definition for LSM303Accelerometer.
 * This class provides a simple wrapper between the hybrid FXOS8700 accelerometer and higher level accelerometer funcitonality.
 */
class LSM303Accelerometer : public MicroBitAccelerometer
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
    LSM303Accelerometer(MicroBitI2C& _i2c, MicroBitPin _int1, CoordinateSpace &coordinateSpace, uint16_t address = LSM303_A_DEFAULT_ADDR, uint16_t id = MICROBIT_ID_ACCELEROMETER);

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
    static int isDetected(MicroBitI2C &i2c, uint16_t address = LSM303_A_DEFAULT_ADDR);

    /**
     * Destructor.
     */
    ~LSM303Accelerometer();

};

#endif

