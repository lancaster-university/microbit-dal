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

#ifdef TARGET_NRF51_CALLIOPE

#ifndef MICROBIT_ACCELEROMETER_H
#define MICROBIT_ACCELEROMETER_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "MicroBitCoordinateSystem.h"
#include "MicroBitI2C.h"
#include "MicroBitPin.h"

#define BMX055_ACC_WHOAMI        0x00   // should return 0xFA
//#define BMX055_ACC_Reserved    0x01
#define BMX055_ACC_D_X_LSB       0x02
#define BMX055_ACC_D_X_MSB       0x03
#define BMX055_ACC_D_Y_LSB       0x04
#define BMX055_ACC_D_Y_MSB       0x05
#define BMX055_ACC_D_Z_LSB       0x06
#define BMX055_ACC_D_Z_MSB       0x07
#define BMX055_ACC_D_TEMP        0x08
#define BMX055_ACC_INT_STATUS_0  0x09
#define BMX055_ACC_INT_STATUS_1  0x0A
#define BMX055_ACC_INT_STATUS_2  0x0B
#define BMX055_ACC_INT_STATUS_3  0x0C
//#define BMX055_ACC_Reserved    0x0D
#define BMX055_ACC_FIFO_STATUS   0x0E
#define BMX055_ACC_PMU_RANGE     0x0F
#define BMX055_ACC_PMU_BW        0x10
#define BMX055_ACC_PMU_LPW       0x11
#define BMX055_ACC_PMU_LOW_POWER 0x12
#define BMX055_ACC_D_HBW         0x13
#define BMX055_ACC_BGW_SOFTRESET 0x14
//#define BMX055_ACC_Reserved    0x15
#define BMX055_ACC_INT_EN_0      0x16
#define BMX055_ACC_INT_EN_1      0x17
#define BMX055_ACC_INT_EN_2      0x18
#define BMX055_ACC_INT_MAP_0     0x19
#define BMX055_ACC_INT_MAP_1     0x1A
#define BMX055_ACC_INT_MAP_2     0x1B
//#define BMX055_ACC_Reserved    0x1C
//#define BMX055_ACC_Reserved    0x1D
#define BMX055_ACC_INT_SRC       0x1E
//#define BMX055_ACC_Reserved    0x1F
#define BMX055_ACC_INT_OUT_CTRL  0x20
#define BMX055_ACC_INT_RST_LATCH 0x21
#define BMX055_ACC_INT_0         0x22
#define BMX055_ACC_INT_1         0x23
#define BMX055_ACC_INT_2         0x24
#define BMX055_ACC_INT_3         0x25
#define BMX055_ACC_INT_4         0x26
#define BMX055_ACC_INT_5         0x27
#define BMX055_ACC_INT_6         0x28
#define BMX055_ACC_INT_7         0x29
#define BMX055_ACC_INT_8         0x2A
#define BMX055_ACC_INT_9         0x2B
#define BMX055_ACC_INT_A         0x2C
#define BMX055_ACC_INT_B         0x2D
#define BMX055_ACC_INT_C         0x2E
#define BMX055_ACC_INT_D         0x2F
#define BMX055_ACC_FIFO_CONFIG_0 0x30
//#define BMX055_ACC_Reserved    0x31
#define BMX055_ACC_PMU_SELF_TEST 0x32
#define BMX055_ACC_TRIM_NVM_CTRL 0x33
#define BMX055_ACC_BGW_SPI3_WDT  0x34
//#define BMX055_ACC_Reserved    0x35
#define BMX055_ACC_OFC_CTRL      0x36
#define BMX055_ACC_OFC_SETTING   0x37
#define BMX055_ACC_OFC_OFFSET_X  0x38
#define BMX055_ACC_OFC_OFFSET_Y  0x39
#define BMX055_ACC_OFC_OFFSET_Z  0x3A
#define BMX055_ACC_TRIM_GPO      0x3B
#define BMX055_ACC_TRIM_GP1      0x3C
//#define BMX055_ACC_Reserved    0x3D
#define BMX055_ACC_FIFO_CONFIG_1 0x3E
#define BMX055_ACC_FIFO_DATA     0x3F

// BMX055 Gyroscope Registers
#define BMX055_GYRO_WHOAMI           0x00  // should return 0x0F
//#define BMX055_GYRO_Reserved       0x01
#define BMX055_GYRO_RATE_X_LSB       0x02
#define BMX055_GYRO_RATE_X_MSB       0x03
#define BMX055_GYRO_RATE_Y_LSB       0x04
#define BMX055_GYRO_RATE_Y_MSB       0x05
#define BMX055_GYRO_RATE_Z_LSB       0x06
#define BMX055_GYRO_RATE_Z_MSB       0x07
//#define BMX055_GYRO_Reserved       0x08
#define BMX055_GYRO_INT_STATUS_0  0x09
#define BMX055_GYRO_INT_STATUS_1  0x0A
#define BMX055_GYRO_INT_STATUS_2  0x0B
#define BMX055_GYRO_INT_STATUS_3  0x0C
//#define BMX055_GYRO_Reserved    0x0D
#define BMX055_GYRO_FIFO_STATUS   0x0E
#define BMX055_GYRO_RANGE         0x0F
#define BMX055_GYRO_BW            0x10
#define BMX055_GYRO_LPM1          0x11
#define BMX055_GYRO_LPM2          0x12
#define BMX055_GYRO_RATE_HBW      0x13
#define BMX055_GYRO_BGW_SOFTRESET 0x14
#define BMX055_GYRO_INT_EN_0      0x15
#define BMX055_GYRO_INT_EN_1      0x16
#define BMX055_GYRO_INT_MAP_0     0x17
#define BMX055_GYRO_INT_MAP_1     0x18
#define BMX055_GYRO_INT_MAP_2     0x19
#define BMX055_GYRO_INT_SRC_1     0x1A
#define BMX055_GYRO_INT_SRC_2     0x1B
#define BMX055_GYRO_INT_SRC_3     0x1C
//#define BMX055_GYRO_Reserved    0x1D
#define BMX055_GYRO_FIFO_EN       0x1E
//#define BMX055_GYRO_Reserved    0x1F
//#define BMX055_GYRO_Reserved    0x20
#define BMX055_GYRO_INT_RST_LATCH 0x21
#define BMX055_GYRO_HIGH_TH_X     0x22
#define BMX055_GYRO_HIGH_DUR_X    0x23
#define BMX055_GYRO_HIGH_TH_Y     0x24
#define BMX055_GYRO_HIGH_DUR_Y    0x25
#define BMX055_GYRO_HIGH_TH_Z     0x26
#define BMX055_GYRO_HIGH_DUR_Z    0x27
//#define BMX055_GYRO_Reserved    0x28
//#define BMX055_GYRO_Reserved    0x29
//#define BMX055_GYRO_Reserved    0x2A
#define BMX055_GYRO_SOC           0x31
#define BMX055_GYRO_A_FOC         0x32
#define BMX055_GYRO_TRIM_NVM_CTRL 0x33
#define BMX055_GYRO_BGW_SPI3_WDT  0x34
//#define BMX055_GYRO_Reserved    0x35
#define BMX055_GYRO_OFC1          0x36
#define BMX055_GYRO_OFC2          0x37
#define BMX055_GYRO_OFC3          0x38
#define BMX055_GYRO_OFC4          0x39
#define BMX055_GYRO_TRIM_GP0      0x3A
#define BMX055_GYRO_TRIM_GP1      0x3B
#define BMX055_GYRO_BIST          0x3C
#define BMX055_GYRO_FIFO_CONFIG_0 0x3D
#define BMX055_GYRO_FIFO_CONFIG_1 0x3E

// BMX055 magnetometer registers
#define BMX055_MAG_WHOAMI         0x40  // should return 0x32
#define BMX055_MAG_Reserved       0x41
#define BMX055_MAG_XOUT_LSB       0x42
#define BMX055_MAG_XOUT_MSB       0x43
#define BMX055_MAG_YOUT_LSB       0x44
#define BMX055_MAG_YOUT_MSB       0x45
#define BMX055_MAG_ZOUT_LSB       0x46
#define BMX055_MAG_ZOUT_MSB       0x47
#define BMX055_MAG_ROUT_LSB       0x48
#define BMX055_MAG_ROUT_MSB       0x49
#define BMX055_MAG_INT_STATUS     0x4A
#define BMX055_MAG_PWR_CNTL1      0x4B
#define BMX055_MAG_PWR_CNTL2      0x4C
#define BMX055_MAG_INT_EN_1       0x4D
#define BMX055_MAG_INT_EN_2       0x4E
#define BMX055_MAG_LOW_THS        0x4F
#define BMX055_MAG_HIGH_THS       0x50
#define BMX055_MAG_REP_XY         0x51
#define BMX055_MAG_REP_Z          0x52
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

// Using the Teensy Mini Add-On board, SDO1 = SDO2 = CSB3 = GND as designed
// Seven-bit device addresses are ACC = 0x18, GYRO = 0x68, MAG = 0x10
#define BMX055_ACC_ADDRESS  0x18   // Address of BMX055 accelerometer
#define BMX055_GYRO_ADDRESS 0x68   // Address of BMX055 gyroscope
#define BMX055_MAG_ADDRESS  0x10   // Address of BMX055 magnetometer
#define MS5637_ADDRESS      0x76   // Address of altimeter

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

enum Gscale {
    GFS_2000DPS = 0,
    GFS_1000DPS,
    GFS_500DPS,
    GFS_250DPS,
    GFS_125DPS
};

enum GODRBW {
    G_2000Hz523Hz = 0, // 2000 Hz ODR and unfiltered (bandwidth 523Hz)
    G_2000Hz230Hz,
    G_1000Hz116Hz,
    G_400Hz47Hz,
    G_200Hz23Hz,
    G_100Hz12Hz,
    G_200Hz64Hz,
    G_100Hz32Hz  // 100 Hz ODR and 32 Hz bandwidth
};

enum MODR {
    MODR_10Hz = 0,   // 10 Hz ODR
    MODR_2Hz     ,   // 2 Hz ODR
    MODR_6Hz     ,   // 6 Hz ODR
    MODR_8Hz     ,   // 8 Hz ODR
    MODR_15Hz    ,   // 15 Hz ODR
    MODR_20Hz    ,   // 20 Hz ODR
    MODR_25Hz    ,   // 25 Hz ODR
    MODR_30Hz        // 30 Hz ODR
};

enum Mmode {
    lowPower         = 0,   // rms noise ~1.0 microTesla, 0.17 mA power
    Regular             ,   // rms noise ~0.6 microTesla, 0.5 mA power
    enhancedRegular     ,   // rms noise ~0.5 microTesla, 0.8 mA power
    highAccuracy            // rms noise ~0.3 microTesla, 4.9 mA power
};

// MS5637 pressure sensor sample rates
#define ADC_256  0x00 // define pressure and temperature conversion rates
#define ADC_512  0x02
#define ADC_1024 0x04
#define ADC_2048 0x06
#define ADC_4096 0x08
#define ADC_8192 0x0A
#define ADC_D1   0x40
#define ADC_D2   0x50

/**
  * Relevant pin assignments
  */
#define MICROBIT_PIN_ACCEL_DATA_READY          CALLIOPE_PIN_ACCEL_INT

/**
  * Status flags
  */
#define MICROBIT_ACCEL_PITCH_ROLL_VALID           0x02
#define MICROBIT_ACCEL_ADDED_TO_IDLE              0x04

/**
  * I2C constants
  */
//#define MMA8653_DEFAULT_ADDR    0x3A

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

#define MMA8653_SAMPLE_RANGES   3
#define MMA8653_SAMPLE_RATES    8

/**
  * Accelerometer events
  */
#define MICROBIT_ACCELEROMETER_EVT_DATA_UPDATE              1

/**
  * Gesture events
  */
#define MICROBIT_ACCELEROMETER_EVT_NONE                     0
#define MICROBIT_ACCELEROMETER_EVT_TILT_UP                  1
#define MICROBIT_ACCELEROMETER_EVT_TILT_DOWN                2
#define MICROBIT_ACCELEROMETER_EVT_TILT_LEFT                3
#define MICROBIT_ACCELEROMETER_EVT_TILT_RIGHT               4
#define MICROBIT_ACCELEROMETER_EVT_FACE_UP                  5
#define MICROBIT_ACCELEROMETER_EVT_FACE_DOWN                6
#define MICROBIT_ACCELEROMETER_EVT_FREEFALL                 7
#define MICROBIT_ACCELEROMETER_EVT_3G                       8
#define MICROBIT_ACCELEROMETER_EVT_6G                       9
#define MICROBIT_ACCELEROMETER_EVT_8G                       10
#define MICROBIT_ACCELEROMETER_EVT_SHAKE                    11

/**
  * Gesture recogniser constants
  */
#define MICROBIT_ACCELEROMETER_REST_TOLERANCE               200
#define MICROBIT_ACCELEROMETER_TILT_TOLERANCE               200
#define MICROBIT_ACCELEROMETER_FREEFALL_TOLERANCE           400
#define MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE              400
#define MICROBIT_ACCELEROMETER_2G_TOLERANCE                 2048
#define MICROBIT_ACCELEROMETER_3G_TOLERANCE                 3072
#define MICROBIT_ACCELEROMETER_6G_TOLERANCE                 6144
#define MICROBIT_ACCELEROMETER_8G_TOLERANCE                 8192
#define MICROBIT_ACCELEROMETER_GESTURE_DAMPING              5
#define MICROBIT_ACCELEROMETER_SHAKE_DAMPING                10 
#define MICROBIT_ACCELEROMETER_SHAKE_RTX                    30

#define MICROBIT_ACCELEROMETER_REST_THRESHOLD               (MICROBIT_ACCELEROMETER_REST_TOLERANCE * MICROBIT_ACCELEROMETER_REST_TOLERANCE)
#define MICROBIT_ACCELEROMETER_FREEFALL_THRESHOLD           (MICROBIT_ACCELEROMETER_FREEFALL_TOLERANCE * MICROBIT_ACCELEROMETER_FREEFALL_TOLERANCE)
#define MICROBIT_ACCELEROMETER_2G_THRESHOLD                 (MICROBIT_ACCELEROMETER_2G_TOLERANCE * MICROBIT_ACCELEROMETER_2G_TOLERANCE)
#define MICROBIT_ACCELEROMETER_3G_THRESHOLD                 (MICROBIT_ACCELEROMETER_3G_TOLERANCE * MICROBIT_ACCELEROMETER_3G_TOLERANCE)
#define MICROBIT_ACCELEROMETER_6G_THRESHOLD                 (MICROBIT_ACCELEROMETER_6G_TOLERANCE * MICROBIT_ACCELEROMETER_6G_TOLERANCE)
#define MICROBIT_ACCELEROMETER_8G_THRESHOLD                 (MICROBIT_ACCELEROMETER_8G_TOLERANCE * MICROBIT_ACCELEROMETER_8G_TOLERANCE)
#define MICROBIT_ACCELEROMETER_SHAKE_COUNT_THRESHOLD        4

struct MMA8653Sample
{
    int16_t         x;
    int16_t         y;
    int16_t         z;
};

struct MMA8653SampleRateConfig
{
    uint32_t        sample_period;
    uint8_t         ctrl_reg1;
};

struct MMA8653SampleRangeConfig
{
    uint8_t         sample_range;
    uint8_t         xyz_data_cfg;
};


extern const MMA8653SampleRangeConfig MMA8653SampleRange[];
extern const MMA8653SampleRateConfig MMA8653SampleRate[];

struct ShakeHistory
{
    uint16_t    shaken:1,
                x:1,
                y:1,
                z:1,
                unused,
                impulse_2,
                impulse_3,
                impulse_6,
                impulse_8,
                count:8;

    uint16_t    timer;
};

/**
 * Class definition for MicroBit Accelerometer.
 *
 * Represents an implementation of the Freescale MMA8653 3 axis accelerometer
 * Also includes basic data caching and on demand activation.
 */
class MicroBitAccelerometer : public MicroBitComponent
{
    uint16_t        address;            // I2C address of this accelerometer.
    uint16_t        samplePeriod;       // The time between samples, in milliseconds.
    uint8_t         sampleRange;        // The sample range of the accelerometer in g.
    MMA8653Sample   sample;             // The last sample read.
    DigitalIn       int1;               // Data ready interrupt.
    float           pitch;              // Pitch of the device, in radians.
    MicroBitI2C&    i2c;                // The I2C interface to use.
    float           roll;               // Roll of the device, in radians.
    uint8_t         sigma;              // the number of ticks that the instantaneous gesture has been stable.
    uint8_t         impulseSigma;       // the number of ticks since an impulse event has been generated.
    uint16_t        lastGesture;        // the last, stable gesture recorded.
    uint16_t        currentGesture;     // the instantaneous, unfiltered gesture detected.
    ShakeHistory    shake;              // State information needed to detect shake events.

    // Specify sensor full scale
    uint8_t OSR    = ADC_8192;         // set pressure amd temperature oversample rate
    uint8_t Gscale = GFS_250DPS;       // set gyro full scale
    uint8_t GODRBW = G_2000Hz523Hz;      // set gyro ODR and bandwidth
    uint8_t Ascale = AFS_2G;           // set accel full scale
    uint8_t ACCBW  = 0x03 | ABW_63Hz;  // Choose bandwidth for accelerometer, need bit 3 = 1 to enable bandwidth choice in enum
    uint8_t Mmode  = Regular;          // Choose magnetometer operation mode
    uint8_t MODR   = MODR_10Hz;        // set magnetometer data rate
    float aRes, gRes, mRes;            // scale resolutions per LSB for the sensors

    public:

    /**
      * Constructor.
      * Create a software abstraction of an accelerometer.
      *
      * @param _i2c an instance of MicroBitI2C used to communicate with the onboard accelerometer.
      *
      * @param address the default I2C address of the accelerometer. Defaults to: MMA8653_DEFAULT_ADDR.
      *
      * @param id the unique EventModel id of this component. Defaults to: MICROBIT_ID_ACCELEROMETER
      *
      * @code
      * MicroBitI2C i2c = MicroBitI2C(I2C_SDA0, I2C_SCL0);
      *
      * MicroBitAccelerometer accelerometer = MicroBitAccelerometer(i2c);
      * @endcode
     */
    MicroBitAccelerometer(MicroBitI2C &_i2c, uint16_t address = BMX055_ACC_ADDRESS, uint16_t id = MICROBIT_ID_ACCELEROMETER);

    /**
      * Configures the accelerometer for G range and sample rate defined
      * in this object. The nearest values are chosen to those defined
      * that are supported by the hardware. The instance variables are then
      * updated to reflect reality.
      *
      * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the accelerometer could not be configured.
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
      * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the read request fails.
      */
    int updateSample();

    /**
      * Attempts to set the sample rate of the accelerometer to the specified value (in ms).
      *
      * @param period the requested time between samples, in milliseconds.
      *
      * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
      *
      * @code
      * // sample rate is now 20 ms.
      * accelerometer.setPeriod(20);
      * @endcode
      *
      * @note The requested rate may not be possible on the hardware. In this case, the
      * nearest lower rate is chosen.
      */
    int setPeriod(int period);

    /**
      * Reads the currently configured sample rate of the accelerometer.
      *
      * @return The time between samples, in milliseconds.
      */
    int getPeriod();

    /**
      * Attempts to set the sample range of the accelerometer to the specified value (in g).
      *
      * @param range The requested sample range of samples, in g.
      *
      * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
      *
      * @code
      * // the sample range of the accelerometer is now 8G.
      * accelerometer.setRange(8);
      * @endcode
      *
      * @note The requested range may not be possible on the hardware. In this case, the
      * nearest lower range is chosen.
      */
    int setRange(int range);

    /**
      * Reads the currently configured sample range of the accelerometer.
      *
      * @return The sample range, in g.
      */
    int getRange();

    /**
      * Attempts to read the 8 bit ID from the accelerometer, this can be used for
      * validation purposes.
      *
      * @return the 8 bit ID returned by the accelerometer, or MICROBIT_I2C_ERROR if the request fails.
      *
      * @code
      * accelerometer.whoAmI();
      * @endcode
      */
    int whoAmI();

    /**
      * Reads the value of the X axis from the latest update retrieved from the accelerometer.
      *
      * @param system The coordinate system to use. By default, a simple cartesian system is provided.
      *
      * @return The force measured in the X axis, in milli-g.
      *
      * @code
      * accelerometer.getX();
      * @endcode
      */
    int getX(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
      * Reads the value of the Y axis from the latest update retrieved from the accelerometer.
      *
      * @return The force measured in the Y axis, in milli-g.
      *
      * @code
      * accelerometer.getY();
      * @endcode
      */
    int getY(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
      * Reads the value of the Z axis from the latest update retrieved from the accelerometer.
      *
      * @return The force measured in the Z axis, in milli-g.
      *
      * @code
      * accelerometer.getZ();
      * @endcode
      */
    int getZ(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
      * Provides a rotation compensated pitch of the device, based on the latest update retrieved from the accelerometer.
      *
      * @return The pitch of the device, in degrees.
      *
      * @code
      * accelerometer.getPitch();
      * @endcode
      */
    int getPitch();

    /**
      * Provides a rotation compensated pitch of the device, based on the latest update retrieved from the accelerometer.
      *
      * @return The pitch of the device, in radians.
      *
      * @code
      * accelerometer.getPitchRadians();
      * @endcode
      */
    float getPitchRadians();

    /**
      * Provides a rotation compensated roll of the device, based on the latest update retrieved from the accelerometer.
      *
      * @return The roll of the device, in degrees.
      *
      * @code
      * accelerometer.getRoll();
      * @endcode
      */
    int getRoll();

    /**
      * Provides a rotation compensated roll of the device, based on the latest update retrieved from the accelerometer.
      *
      * @return The roll of the device, in radians.
      *
      * @code
      * accelerometer.getRollRadians();
      * @endcode
      */
    float getRollRadians();

    /**
      * Retrieves the last recorded gesture.
      *
      * @return The last gesture that was detected.
      *
      * Example:
      * @code
      * MicroBitDisplay display;
      *
      * if (accelerometer.getGesture() == SHAKE)
      *     display.scroll("SHAKE!");
      * @endcode
      */
    uint16_t getGesture();

    /**
      * A periodic callback invoked by the fiber scheduler idle thread.
      *
      * Internally calls updateSample().
      */
    virtual void idleTick();

    /**
      * Destructor for MicroBitButton, where we deregister this instance from the array of fiber components.
      */
    ~MicroBitAccelerometer();

    private:

    void writeByte(char id, char addr, char value);
    char readByte(char id, char addr);
    void readBytes(uint8_t id, uint8_t addr, uint8_t num, uint8_t *rawData);
    void readAccelData(int16_t * destination);


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
    int writeCommand(uint8_t reg, uint8_t value);

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
    int readCommand(uint8_t reg, uint8_t* buffer, int length);

    /**
      * Recalculate roll and pitch values for the current sample.
      *
      * @note We only do this at most once per sample, as the necessary trigonemteric functions are rather
      *       heavyweight for a CPU without a floating point unit.
      */
    void recalculatePitchRoll();

    /**
      * Updates the basic gesture recognizer. This performs instantaneous pose recognition, and also some low pass filtering to promote
      * stability.
      */
    void updateGesture();

    /**
      * A service function.
      * It calculates the current scalar acceleration of the device (x^2 + y^2 + z^2).
      * It does not, however, square root the result, as this is a relatively high cost operation.
      *
      * This is left to application code should it be needed.
      *
      * @return the sum of the square of the acceleration of the device across all axes.
      */
    int instantaneousAccelerationSquared();

    /**
     * Service function.
     * Determines a 'best guess' posture of the device based on instantaneous data.
     *
     * This makes no use of historic data, and forms this input to the filter implemented in updateGesture().
     *
     * @return A 'best guess' of the current posture of the device, based on instanataneous data.
     */
    uint16_t instantaneousPosture();
};

#endif

#endif // TARGET_NRF51_CALLIOPE