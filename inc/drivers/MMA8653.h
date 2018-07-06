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

#ifndef MMA8653_H
#define MMA8653_H

#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "CoordinateSystem.h"
#include "MicroBitAccelerometer.h"
#include "MicroBitI2C.h"
#include "MicroBitUtil.h"

/**
  * Relevant pin assignments
  */
#define MICROBIT_PIN_ACCEL_DATA_READY          P0_28

/**
  * I2C constants
  */
#define MMA8653_DEFAULT_ADDR    0x3A

/**
  * MMA8653 Register map (partial)
  */
#define MMA8653_STATUS          0x00
#define MMA8653_OUT_X_MSB       0x01
#define MMA8653_WHOAMI          0x0D
#define MMA8653_XYZ_DATA_CFG    0x0E
#define MMA8653_CTRL_REG1       0x2A
#define MMA8653_CTRL_REG2       0x2B
#define MMA8653_CTRL_REG3       0x2C
#define MMA8653_CTRL_REG4       0x2D
#define MMA8653_CTRL_REG5       0x2E


/**
  * MMA8653 constants
  */
#define MMA8653_WHOAMI_VAL      0x5A

/**
 * Class definition for MMA8653.
 * This class provides a simple wrapper between the hybrid FXOS8700 accelerometer and higher level accelerometer funcitonality.
 */
class MMA8653 : public MicroBitAccelerometer
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
    MMA8653(MicroBitI2C& _i2c, MicroBitPin _int1, CoordinateSpace &coordinateSpace, uint16_t address = MMA8653_DEFAULT_ADDR, uint16_t id = MICROBIT_ID_ACCELEROMETER);

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
    static int isDetected(MicroBitI2C &i2c, uint16_t address = MMA8653_DEFAULT_ADDR);


    /**
     * Destructor.
     */
    ~MMA8653();

};

#endif

