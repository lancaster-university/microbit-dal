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

#ifdef TARGET_NRF51_MICROBIT

#ifndef MICROBIT_ACCELEROMETER_H
#define MICROBIT_ACCELEROMETER_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "MicroBitCoordinateSystem.h"
#include "MicroBitI2C.h"

/**
  * Relevant pin assignments
  */
#define MICROBIT_PIN_ACCEL_DATA_READY          P0_28

/**
  * Status flags
  */
#define MICROBIT_ACCEL_PITCH_ROLL_VALID           0x02
#define MICROBIT_ACCEL_ADDED_TO_IDLE              0x04

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
#define MICROBIT_ACCELEROMETER_3G_TOLERANCE                 3072
#define MICROBIT_ACCELEROMETER_6G_TOLERANCE                 6144
#define MICROBIT_ACCELEROMETER_8G_TOLERANCE                 8192
#define MICROBIT_ACCELEROMETER_GESTURE_DAMPING              5
#define MICROBIT_ACCELEROMETER_SHAKE_DAMPING                10 
#define MICROBIT_ACCELEROMETER_SHAKE_RTX                    30

#define MICROBIT_ACCELEROMETER_REST_THRESHOLD               (MICROBIT_ACCELEROMETER_REST_TOLERANCE * MICROBIT_ACCELEROMETER_REST_TOLERANCE)
#define MICROBIT_ACCELEROMETER_FREEFALL_THRESHOLD           (MICROBIT_ACCELEROMETER_FREEFALL_TOLERANCE * MICROBIT_ACCELEROMETER_FREEFALL_TOLERANCE)
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
    MicroBitAccelerometer(MicroBitI2C &_i2c, uint16_t address = MMA8653_DEFAULT_ADDR, uint16_t id = MICROBIT_ID_ACCELEROMETER);

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
#endif // TARGET_NRF51_MICROBIT