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

#ifndef FXOS8700_H
#define FXOS8700_H

#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "MicroBitPin.h"
#include "MicroBitI2C.h"
#include "MicroBitAccelerometer.h"
#include "MicroBitCompass.h"
#include "CoordinateSystem.h"
#include "MicroBitUtil.h"

/**
  * I2C constants
  */
#define FXOS8700_DEFAULT_ADDR       0x3C

/**
  * FXOS8700 Register map
  */
#define FXOS8700_STATUS_REG         0x00
#define FXOS8700_OUT_X_MSB          0x01
#define FXOS8700_OUT_X_LSB          0x02
#define FXOS8700_OUT_Y_MSB          0x03
#define FXOS8700_OUT_Y_LSB          0x04
#define FXOS8700_OUT_Z_MSB          0x05
#define FXOS8700_OUT_Z_LSB          0x06
#define FXOS8700_F_SETUP            0x09
#define FXOS8700_TRIG_CFG           0x0A
#define FXOS8700_SYSMOD             0x0B
#define FXOS8700_INT_SOURCE         0x0C
#define FXOS8700_WHO_AM_I           0x0D
#define FXOS8700_XYZ_DATA_CFG       0x0E
#define FXOS8700_HP_FILTER_CUTOFF   0x0F
#define FXOS8700_PL_STATUS          0x10
#define FXOS8700_PL_CFG             0x11
#define FXOS8700_PL_COUNT           0x12
#define FXOS8700_PL_BF_ZCOMP        0x13
#define FXOS8700_PL_THS_REG         0x14
#define FXOS8700_A_FFMT_CFG         0x15
#define FXOS8700_A_FFMT_SRC         0x16
#define FXOS8700_A_FFMT_THS         0x17
#define FXOS8700_A_FFMT_COUNT       0x18
#define FXOS8700_TRANSIENT_CFG      0x1D
#define FXOS8700_TRANSIENT_SRC      0x1E
#define FXOS8700_TRANSIENT_THS      0x1F
#define FXOS8700_TRANSIENT_COUNT    0x20
#define FXOS8700_PULSE_CFG          0x21
#define FXOS8700_PULSE_SRC          0x22
#define FXOS8700_PULSE_THSX         0x23
#define FXOS8700_PULSE_THSY         0x24
#define FXOS8700_PULSE_THSZ         0x25
#define FXOS8700_PULSE_TMLT         0x26
#define FXOS8700_PULSE_LTCY         0x27
#define FXOS8700_PULSE_WIND         0x28
#define FXOS8700_ASLP_COUNT         0x29
#define FXOS8700_CTRL_REG1          0x2A
#define FXOS8700_CTRL_REG2          0x2B
#define FXOS8700_CTRL_REG3          0x2C
#define FXOS8700_CTRL_REG4          0x2D
#define FXOS8700_CTRL_REG5          0x2E
#define FXOS8700_OFF_X              0x2F
#define FXOS8700_OFF_Y              0x30
#define FXOS8700_OFF_Z              0x31
#define FXOS8700_M_DR_STATUS        0x32
#define FXOS8700_M_OUT_X_MSB        0x33
#define FXOS8700_M_OUT_X_LSB        0x34
#define FXOS8700_M_OUT_Y_MSB        0x35
#define FXOS8700_M_OUT_Y_LSB        0x36
#define FXOS8700_M_OUT_Z_MSB        0x37
#define FXOS8700_M_OUT_Z_LSB        0x38
#define FXOS8700_CMP_X_MSB          0x39
#define FXOS8700_CMP_X_LSB          0x3A
#define FXOS8700_CMP_Y_MSB          0x3B
#define FXOS8700_CMP_Y_LSB          0x3C
#define FXOS8700_CMP_Z_MSB          0x3D
#define FXOS8700_CMP_Z_LSB          0x3E
#define FXOS8700_M_OFF_X_MSB        0x3F
#define FXOS8700_M_OFF_X_LSB        0x40
#define FXOS8700_M_OFF_Y_MSB        0x41
#define FXOS8700_M_OFF_Y_LSB        0x42
#define FXOS8700_M_OFF_Z_MSB        0x43
#define FXOS8700_M_OFF_Z_LSB        0x44
#define FXOS8700_MAX_X_MSB          0x45
#define FXOS8700_MAX_X_LSB          0x46
#define FXOS8700_MAX_Y_MSB          0x47
#define FXOS8700_MAX_Y_LSB          0x48
#define FXOS8700_MAX_Z_MSB          0x49
#define FXOS8700_MAX_Z_LSB          0x4A
#define FXOS8700_MIN_X_MSB          0x4B
#define FXOS8700_MIN_X_LSB          0x4C
#define FXOS8700_MIN_Y_MSB          0x4D
#define FXOS8700_MIN_Y_LSB          0x4E
#define FXOS8700_MIN_Z_MSB          0x4F
#define FXOS8700_MIN_Z_LSB          0x50
#define FXOS8700_TEMP               0x51
#define FXOS8700_M_THS_CFG          0x52
#define FXOS8700_M_THS_SRC          0x53
#define FXOS8700_M_THS_X_MSB        0x54
#define FXOS8700_M_THS_X_LSB        0x55
#define FXOS8700_M_THS_Y_MSB        0x56
#define FXOS8700_M_THS_Y_LSB        0x57
#define FXOS8700_M_THS_Z_MSB        0x58
#define FXOS8700_M_THS_Z_LSB        0x59
#define FXOS8700_M_THS_COUNT        0x5A
#define FXOS8700_M_CTRL_REG1        0x5B
#define FXOS8700_M_CTRL_REG2        0x5C
#define FXOS8700_M_CTRL_REG3        0x5D
#define FXOS8700_M_INT_SRC          0x5E
#define FXOS8700_A_VECM_CFG         0x5F
#define FXOS8700_A_VECM_THS_MSB     0x60
#define FXOS8700_A_VECM_THS_LSB     0x61
#define FXOS8700_A_VECM_CNT         0x62
#define FXOS8700_A_VECM_INITX_MSB   0x63
#define FXOS8700_A_VECM_INITX_LSB   0x64
#define FXOS8700_A_VECM_INITY_MSB   0x65
#define FXOS8700_A_VECM_INITY_LSB   0x66
#define FXOS8700_A_VECM_INITZ_MSB   0x67
#define FXOS8700_A_VECM_INITZ_LSB   0x68
#define FXOS8700_M_VECM_CFG         0x69
#define FXOS8700_M_VECM_THS_MSB     0x6A
#define FXOS8700_M_VECM_THS_LSB     0x6B
#define FXOS8700_M_VECM_CNT         0x6C
#define FXOS8700_M_VECM_INITX_MSB   0x6D
#define FXOS8700_M_VECM_INITX_LSB   0x6E
#define FXOS8700_M_VECM_INITY_MSB   0x6F
#define FXOS8700_M_VECM_INITY_LSB   0x70
#define FXOS8700_M_VECM_INITZ_MSB   0x71
#define FXOS8700_M_VECM_INITZ_LSB   0x72
#define FXOS8700_A_FFMT_THS_X_MSB   0x73
#define FXOS8700_A_FFMT_THS_X_LSB   0x74
#define FXOS8700_A_FFMT_THS_Y_MSB   0x75
#define FXOS8700_A_FFMT_THS_Y_LSB   0x76
#define FXOS8700_A_FFMT_THS_Z_MSB   0x77
#define FXOS8700_A_FFMT_THS_Z_LSB   0x78


/**
 * FXOS8700 constants
 */
#define FXOS8700_WHOAMI_VAL      0xC7

/**
  * Term to convert sample data into SI units. 
  */
#define FXOS8700_NORMALIZE_SAMPLE(x) (100 * (int)(x))


/**
 * Class definition for an FXSO8700 hybrid Accelerometer/Magnetometer
 */
class FXOS8700 : public MicroBitAccelerometer, public MicroBitCompass
{
    MicroBitI2C&            i2c;                    // The I2C interface to use.
    MicroBitPin             int1;                   // Data ready interrupt.
    uint16_t                address;                // I2C address of this accelerometer.

    public:

    /**
     * Constructor.
     * Create a software abstraction of an accelerometer.
     *
     * @param _i2c an instance of I2C used to communicate with the onboard accelerometer.
     * @param _int1 a pin connected to the INT1 interrupt source of the sensor.
     * @param address the default I2C address of the accelerometer. Defaults to: FXOS8700_DEFAULT_ADDR.
     *
     */
    FXOS8700(MicroBitI2C &_i2c, MicroBitPin _int1, CoordinateSpace &coordinateSpace, uint16_t address = FXOS8700_DEFAULT_ADDR, uint16_t aid = MICROBIT_ID_ACCELEROMETER, uint16_t cid = MICROBIT_ID_COMPASS);

    /**
     * Configures the accelerometer for G range and sample rate defined
     * in this object. The nearest values are chosen to those defined
     * that are supported by the hardware. The instance variables are then
     * updated to reflect reality.
     *
     * @return DEVICE_OK on success, DEVICE_I2C_ERROR if the accelerometer could not be configured.
     */
    int configure();

    /**
     * Reads the acceleration data from the accelerometer, and stores it in our buffer.
     * This only happens if the accelerometer indicates that it has new data via int1.
     *
     * On first use, this member function will attempt to add this component to the
     * list of fiber components in order to constantly update the values stored
     * by this object.
     *
     * This technique is called lazy instantiation, and it means that we do not
     * obtain the overhead from non-chalantly adding this component to fiber components.
     *
     * @return DEVICE_OK on success, DEVICE_I2C_ERROR if the read request fails.
     */
    virtual int requestUpdate();

    /**
     * Attempts to read the 8 bit WHO_AM_I value from the accelerometer
     *
     * @return true if the WHO_AM_I value is succesfully read. false otherwise.
     */
    static int isDetected(MicroBitI2C &i2c, uint16_t address = FXOS8700_DEFAULT_ADDR);

    /**
     * A periodic callback invoked by the fiber scheduler idle thread.
     *
     * Internally calls updateSample().
     */
    virtual void idleTick();

    /**
     * Destructor.
     */
    ~FXOS8700();

};

#endif
