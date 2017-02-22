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

#ifndef MICROBIT_COMPASS_H
#define MICROBIT_COMPASS_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "MicroBitCoordinateSystem.h"
#ifdef TARGET_NRF51_CALLIOPE
#include "MicroBitAccelerometer-bmx.h"
#else
#include "MicroBitAccelerometer.h"
#endif
#include "MicroBitStorage.h"

/**
  * Relevant pin assignments
  */
#define MICROBIT_PIN_COMPASS_DATA_READY          P0_29

/**
  * I2C constants
  */
#define MAG3110_DEFAULT_ADDR    0x1D

/**
  * MAG3110 Register map
  */
#define MAG_DR_STATUS 0x00
#define MAG_OUT_X_MSB 0x01
#define MAG_OUT_X_LSB 0x02
#define MAG_OUT_Y_MSB 0x03
#define MAG_OUT_Y_LSB 0x04
#define MAG_OUT_Z_MSB 0x05
#define MAG_OUT_Z_LSB 0x06
#define MAG_WHOAMI    0x07
#define MAG_SYSMOD    0x08
#define MAG_OFF_X_MSB 0x09
#define MAG_OFF_X_LSB 0x0A
#define MAG_OFF_Y_MSB 0x0B
#define MAG_OFF_Y_LSB 0x0C
#define MAG_OFF_Z_MSB 0x0D
#define MAG_OFF_Z_LSB 0x0E
#define MAG_DIE_TEMP  0x0F
#define MAG_CTRL_REG1 0x10
#define MAG_CTRL_REG2 0x11

/**
  * Configuration options
  */
struct MAG3110SampleRateConfig
{
    uint32_t        sample_period;
    uint8_t         ctrl_reg1;
};

extern const MAG3110SampleRateConfig MAG3110SampleRate[];

#define MAG3110_SAMPLE_RATES                    11

/**
  * Compass events
  */
#define MICROBIT_COMPASS_EVT_CAL_REQUIRED       1               // DEPRECATED
#define MICROBIT_COMPASS_EVT_CAL_START          2               // DEPRECATED
#define MICROBIT_COMPASS_EVT_CAL_END            3               // DEPRECATED

#define MICROBIT_COMPASS_EVT_DATA_UPDATE        4
#define MICROBIT_COMPASS_EVT_CONFIG_NEEDED      5
#define MICROBIT_COMPASS_EVT_CALIBRATE          6

/**
  * Status Bits
  */
#define MICROBIT_COMPASS_STATUS_CALIBRATED      2
#define MICROBIT_COMPASS_STATUS_CALIBRATING     4
#define MICROBIT_COMPASS_STATUS_ADDED_TO_IDLE   8

/**
  * Term to convert sample data into SI units
  */
#define MAG3110_NORMALIZE_SAMPLE(x) (100*x)

/**
  * MAG3110 MAGIC ID value
  * Returned from the MAG_WHO_AM_I register for ID purposes.
  */
#define MAG3110_WHOAMI_VAL 0xC4

struct CompassSample
{
    int     x;
    int     y;
    int     z;

    CompassSample()
    {
        this->x = 0;
        this->y = 0;
        this->z = 0;
    }

    CompassSample(int x, int y, int z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    bool operator==(const CompassSample& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const CompassSample& other) const
    {
        return !(x == other.x && y == other.y && z == other.z);
    }
};

/**
  * Class definition for MicroBit Compass.
  *
  * Represents an implementation of the Freescale MAG3110 I2C Magnetmometer.
  * Also includes basic caching, calibration and on demand activation.
  */
class MicroBitCompass : public MicroBitComponent
{
    uint16_t                address;                  // I2C address of the magnetmometer.
    uint16_t                samplePeriod;             // The time between samples, in millseconds.

    CompassSample           average;                  // Centre point of sample data.
    CompassSample           sample;                   // The latest sample data recorded.
    DigitalIn               int1;                     // Data ready interrupt.
    MicroBitI2C&		    i2c;                      // The I2C interface the sensor is connected to.
    MicroBitAccelerometer*  accelerometer;            // The accelerometer to use for tilt compensation.
    MicroBitStorage*        storage;                  // An instance of MicroBitStorage used for persistence.

    public:

    /**
      * Constructor.
      * Create a software representation of an e-compass.
      *
      * @param _i2c an instance of i2c, which the compass is accessible from.
      *
      * @param _accelerometer an instance of the accelerometer, used for tilt compensation.
      *
      * @param _storage an instance of MicroBitStorage, used to persist calibration data across resets.
      *
      * @param address the default address for the compass register on the i2c bus. Defaults to MAG3110_DEFAULT_ADDR.
      *
      * @param id the ID of the new MicroBitCompass object. Defaults to MAG3110_DEFAULT_ADDR.
      *
      * @code
      * MicroBitI2C i2c(I2C_SDA0, I2C_SCL0);
      *
      * MicroBitAccelerometer accelerometer(i2c);
      *
      * MicroBitStorage storage;
      *
      * MicroBitCompass compass(i2c, accelerometer, storage);
      * @endcode
      */
    MicroBitCompass(MicroBitI2C& _i2c, MicroBitAccelerometer& _accelerometer, MicroBitStorage& _storage, uint16_t address = MAG3110_DEFAULT_ADDR, uint16_t id = MICROBIT_ID_COMPASS);

    /**
      * Constructor.
      * Create a software representation of an e-compass.
      *
      * @param _i2c an instance of i2c, which the compass is accessible from.
      *
      * @param _accelerometer an instance of the accelerometer, used for tilt compensation.
      *
      * @param address the default address for the compass register on the i2c bus. Defaults to MAG3110_DEFAULT_ADDR.
      *
      * @param id the ID of the new MicroBitCompass object. Defaults to MAG3110_DEFAULT_ADDR.
      *
      * @code
      * MicroBitI2C i2c(I2C_SDA0, I2C_SCL0);
      *
      * MicroBitAccelerometer accelerometer(i2c);
      *
      * MicroBitCompass compass(i2c, accelerometer, storage);
      * @endcode
      */
    MicroBitCompass(MicroBitI2C& _i2c, MicroBitAccelerometer& _accelerometer, uint16_t address = MAG3110_DEFAULT_ADDR, uint16_t id = MICROBIT_ID_COMPASS);

    /**
      * Constructor.
      * Create a software representation of an e-compass.
      *
      * @param _i2c an instance of i2c, which the compass is accessible from.
      *
      * @param _storage an instance of MicroBitStorage, used to persist calibration data across resets.
      *
      * @param address the default address for the compass register on the i2c bus. Defaults to MAG3110_DEFAULT_ADDR.
      *
      * @param id the ID of the new MicroBitCompass object. Defaults to MAG3110_DEFAULT_ADDR.
      *
      * @code
      * MicroBitI2C i2c(I2C_SDA0, I2C_SCL0);
      *
      * MicroBitStorage storage;
      *
      * MicroBitCompass compass(i2c, storage);
      * @endcode
      */
    MicroBitCompass(MicroBitI2C& _i2c, MicroBitStorage& _storage, uint16_t address = MAG3110_DEFAULT_ADDR, uint16_t id = MICROBIT_ID_COMPASS);

    /**
      * Constructor.
      * Create a software representation of an e-compass.
      *
      * @param _i2c an instance of i2c, which the compass is accessible from.
      *
      * @param address the default address for the compass register on the i2c bus. Defaults to MAG3110_DEFAULT_ADDR.
      *
      * @param id the ID of the new MicroBitCompass object. Defaults to MAG3110_DEFAULT_ADDR.
      *
      * @code
      * MicroBitI2C i2c(I2C_SDA0, I2C_SCL0);
      *
      * MicroBitCompass compass(i2c);
      * @endcode
      */
    MicroBitCompass(MicroBitI2C& _i2c, uint16_t address = MAG3110_DEFAULT_ADDR, uint16_t id = MICROBIT_ID_COMPASS);

    /**
      * Configures the compass for the sample rate defined in this object.
      * The nearest values are chosen to those defined that are supported by the hardware.
      * The instance variables are then updated to reflect reality.
      *
      * @return MICROBIT_OK or MICROBIT_I2C_ERROR if the magnetometer could not be configured.
      */
    int configure();

    /**
      * Attempts to set the sample rate of the compass to the specified value (in ms).
      *
      * @param period the requested time between samples, in milliseconds.
      *
      * @return MICROBIT_OK or MICROBIT_I2C_ERROR if the magnetometer could not be updated.
      *
      * @code
      * // sample rate is now 20 ms.
      * compass.setPeriod(20);
      * @endcode
      *
      * @note The requested rate may not be possible on the hardware. In this case, the
      * nearest lower rate is chosen.
      */
    int setPeriod(int period);

    /**
      * Reads the currently configured sample rate of the compass.
      *
      * @return The time between samples, in milliseconds.
      */
    int getPeriod();

    /**
      * Gets the current heading of the device, relative to magnetic north.
      *
      * If the compass is not calibrated, it will raise the MICROBIT_COMPASS_EVT_CALIBRATE event.
      *
      * Users wishing to implement their own calibration algorithms should listen for this event,
      * using MESSAGE_BUS_LISTENER_IMMEDIATE model. This ensures that calibration is complete before
      * the user program continues.
      *
      * @return the current heading, in degrees. Or MICROBIT_CALIBRATION_IN_PROGRESS if the compass is calibrating.
      *
      * @code
      * compass.heading();
      * @endcode
      */
    int heading();

    /**
      * Attempts to read the 8 bit ID from the magnetometer, this can be used for
      * validation purposes.
      *
      * @return the 8 bit ID returned by the magnetometer, or MICROBIT_I2C_ERROR if the request fails.
      *
      * @code
      * compass.whoAmI();
      * @endcode
      */
    int whoAmI();

    /**
      * Reads the value of the X axis from the latest update retrieved from the magnetometer.
      *
      * @param system The coordinate system to use. By default, a simple cartesian system is provided.
      *
      * @return The magnetic force measured in the X axis, in nano teslas.
      *
      * @code
      * compass.getX();
      * @endcode
      */
    int getX(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
      * Reads the value of the Y axis from the latest update retrieved from the magnetometer.
      *
      * @param system The coordinate system to use. By default, a simple cartesian system is provided.
      *
      * @return The magnetic force measured in the Y axis, in nano teslas.
      *
      * @code
      * compass.getY();
      * @endcode
      */
    int getY(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
      * Reads the value of the Z axis from the latest update retrieved from the magnetometer.
      *
      * @param system The coordinate system to use. By default, a simple cartesian system is provided.
      *
      * @return The magnetic force measured in the Z axis, in nano teslas.
      *
      * @code
      * compass.getZ();
      * @endcode
      */
    int getZ(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
      * Determines the overall magnetic field strength based on the latest update from the magnetometer.
      *
      * @return The magnetic force measured across all axis, in nano teslas.
      *
      * @code
      * compass.getFieldStrength();
      * @endcode
      */
    int getFieldStrength();

    /**
      * Reads the current die temperature of the compass.
      *
      * @return the temperature in degrees celsius, or MICROBIT_I2C_ERROR if the temperature reading could not be retreived
      *         from the accelerometer.
      */
    int readTemperature();

    /**
      * Perform a calibration of the compass.
      *
      * This method will be called automatically if a user attempts to read a compass value when
      * the compass is uncalibrated. It can also be called at any time by the user.
      *
      * The method will only return once the compass has been calibrated.
      *
      * @return MICROBIT_OK, MICROBIT_I2C_ERROR if the magnetometer could not be accessed,
      * or MICROBIT_CALIBRATION_REQUIRED if the calibration algorithm failed to complete successfully.
      *
      * @note THIS MUST BE CALLED TO GAIN RELIABLE VALUES FROM THE COMPASS
      */
    int calibrate();

    /**
      * Configure the compass to use the calibration data that is supplied to this call.
      *
      * Calibration data is comprised of the perceived zero offset of each axis of the compass.
      *
      * After calibration this should now take into account trimming errors in the magnetometer,
      * and any "hard iron" offsets on the device.
      *
      * @param calibration A CompassSample containing the offsets for the x, y and z axis.
      */
    void setCalibration(CompassSample calibration);

    /**
      * Provides the calibration data currently in use by the compass.
      *
      * More specifically, the x, y and z zero offsets of the compass.
      *
      * @return calibration A CompassSample containing the offsets for the x, y and z axis.
      */
    CompassSample getCalibration();

    /**
      * Updates the local sample, only if the compass indicates that
      * data is stale.
      *
      * @note Can be used to trigger manual updates, if the device is running without a scheduler.
      *       Also called internally by all get[X,Y,Z]() member functions.
      */
    int updateSample();

    /**
      * Periodic callback from MicroBit idle thread.
      *
      * Calls updateSample().
      */
    virtual void idleTick();

    /**
      * Returns 0 or 1. 1 indicates that the compass is calibrated, zero means the compass requires calibration.
      */
    int isCalibrated();

    /**
      * Returns 0 or 1. 1 indicates that the compass is calibrating, zero means the compass is not currently calibrating.
      */
    int isCalibrating();

    /**
      * Clears the calibration held in persistent storage, and sets the calibrated flag to zero.
      */
    void clearCalibration();

    /**
      * Destructor for MicroBitCompass, where we deregister this instance from the array of fiber components.
      */
    ~MicroBitCompass();

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
      * Issues a read of a given address, and returns the value.
      *
      * Blocks the calling thread until complete.
      *
      * @param reg The address of the 16 bit register to access.
      *
      * @return The register value, interpreted as a 16 but signed value, or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
      */
    int read16(uint8_t reg);

    /**
      * Issues a read of a given address, and returns the value.
      *
      * Blocks the calling thread until complete.
      *
      * @param reg The address of the 16 bit register to access.
      *
      * @return The register value, interpreted as a 8 bit unsigned value, or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
      */
    int read8(uint8_t reg);

    /**
      * Calculates a tilt compensated bearing of the device, using the accelerometer.
      */
    int tiltCompensatedBearing();

    /**
      * Calculates a non-tilt compensated bearing of the device.
      */
    int basicBearing();

    /**
      * An initialisation member function used by the many constructors of MicroBitCompass.
      *
      * @param id the unique identifier for this compass instance.
      *
      * @param address the base address of the magnetometer on the i2c bus.
      */
    void init(uint16_t id, uint16_t address);
};

#endif
