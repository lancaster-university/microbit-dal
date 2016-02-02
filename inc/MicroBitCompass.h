#ifndef MICROBIT_COMPASS_H
#define MICROBIT_COMPASS_H

#include "mbed.h"
#include "MicroBitComponent.h"
#include "MicroBitCoordinateSystem.h"

/**
  * Relevant pin assignments
  */
#define MICROBIT_PIN_COMPASS_DATA_READY          P0_29

/*
 * I2C constants
 */
#define MAG3110_DEFAULT_ADDR    0x1D

/*
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

/*
 * Compass events
 */
#define MICROBIT_COMPASS_EVT_CAL_REQUIRED       1               // DEPRECATED
#define MICROBIT_COMPASS_EVT_CAL_START          2               // DEPRECATED
#define MICROBIT_COMPASS_EVT_CAL_END            3               // DEPRECATED

#define MICROBIT_COMPASS_EVT_DATA_UPDATE        4
#define MICROBIT_COMPASS_EVT_CONFIG_NEEDED      5
#define MICROBIT_COMPASS_EVT_CALIBRATE          6

/*
 * Status Bits
 */
#define MICROBIT_COMPASS_STATUS_CALIBRATED      1
#define MICROBIT_COMPASS_STATUS_CALIBRATING     2

/*
 * Term to convert sample data into SI units
 */
#define MAG3110_NORMALIZE_SAMPLE(x) (100*x)

/*
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
    /**
      * Unique, enumerated ID for this component.
      * Used to track asynchronous events in the event bus.
      */

    uint16_t            address;                  // I2C address of the magnetmometer.
    uint16_t            samplePeriod;             // The time between samples, in millseconds.

    CompassSample       average;                  // Centre point of sample data.
    CompassSample       sample;                   // The latest sample data recorded.
    DigitalIn           int1;                     // Data ready interrupt.

    public:

    /**
      * Constructor.
      * Create a compass representation with the given ID.
      * @param id the event ID of the compass object.
      * @param address the default address for the compass register
      *
      * Example:
      * @code
      * compass(MICROBIT_ID_COMPASS, MAG3110_DEFAULT_ADDR);
      * @endcode
      *
      * Possible Events for the compass are as follows:
      * @code
      * MICROBIT_COMPASS_EVT_CAL_REQUIRED   // triggered when no magnetometer data is available in persistent storage
      * MICROBIT_COMPASS_EVT_CAL_START      // triggered when calibration has begun
      * MICROBIT_COMPASS_EVT_CAL_END        // triggered when calibration has finished.
      * @endcode
      */
    MicroBitCompass(uint16_t id, uint16_t address);

    /**
     * Configures the compass for the sample rate defined
     * in this object. The nearest values are chosen to those defined
     * that are supported by the hardware. The instance variables are then
     * updated to reflect reality.
     * @return MICROBIT_OK or MICROBIT_I2C_ERROR if the magnetometer could not be configured.
     */
    int configure();

    /**
     * Attempts to set the sample rate of the compass to the specified value (in ms).
     * n.b. the requested rate may not be possible on the hardware. In this case, the
     * nearest lower rate is chosen.
     * @param period the requested time between samples, in milliseconds.
     * @return MICROBIT_OK or MICROBIT_I2C_ERROR if the magnetometer could not be updated.
     */
    int setPeriod(int period);

    /**
      * Reads the currently configured sample rate of the compass.
      * @return The time between samples, in milliseconds.
      */
    int getPeriod();

    /**
     * Gets the current heading of the device, relative to magnetic north.
     * If the compass is not calibrated, it will raise the MICROBIT_COMPASS_EVT_CALIBRATE event.
     * Users wishing to implement their own calibration algorithms should listen for this event,
     * using MESSAGE_BUS_LISTENER_IMMEDIATE model. This ensures that calibration is complete before
     * the user program continues.
     *
     * @return the current heading, in degrees. Or MICROBIT_CALIBRATION_IN_PROGRESS if the compass is calibrating.
     *
     * Example:
     * @code
     * uBit.compass.heading();
     * @endcode
     */
    int heading();

    /**
      * Attempts to determine the 8 bit ID from the magnetometer.
      * @return the id of the compass (magnetometer), or MICROBIT_I2C_ERROR if the magnetometer could not be updated.
      *
      * Example:
      * @code
      * uBit.compass.whoAmI();
      * @endcode
      */
    int whoAmI();

    /**
     * Reads the X axis value of the latest update from the compass.
     * @return The magnetic force measured in the X axis, in nano teslas.
     *
     * Example:
     * @code
     * uBit.compass.getX();
     * @endcode
     */
    int getX(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
     * Reads the Y axis value of the latest update from the compass.
     * @return The magnetic force measured in the Y axis, in nano teslas.
     *
     * Example:
     * @code
     * uBit.compass.getY();
     * @endcode
     */
    int getY(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
     * Reads the Z axis value of the latest update from the compass.
     * @return The magnetic force measured in the Z axis, in nano teslas.
     *
     * Example:
     * @code
     * uBit.compass.getZ();
     * @endcode
     */
    int getZ(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
     * Determines the overall magnetic field strength based on the latest update from the compass.
     * @return The magnetic force measured across all axes, in nano teslas.
     *
     * Example:
     * @code
     * uBit.compass.getFieldStrength();
     * @endcode
     */
    int getFieldStrength();

    /**
      * Reads the current die temperature of the compass.
      * @return the temperature in degrees celsius, or MICROBIT_I2C_ERROR if the magnetometer could not be updated.
      */
    int readTemperature();

    /**
     * Perform a calibration of the compass.
     *
     * This method will be called automatically if a user attempts to read a compass value when
     * the compass is uncalibrated. It can also be called at any time by the user.
     *
     * Any old calibration data is deleted.
     * The method will only return once the compass has been calibrated.
     *
     * @return MICROBIT_OK, MICROBIT_I2C_ERROR if the magnetometer could not be accessed,
     * or MICROBIT_CALIBRATION_REQUIRED if the calibration algorithm failed to complete succesfully.
     * @note THIS MUST BE CALLED TO GAIN RELIABLE VALUES FROM THE COMPASS
     */
    int calibrate();

    /**
      * Perform the asynchronous calibration of the compass.
      * This will fire MICROBIT_COMPASS_EVT_CAL_START and MICROBIT_COMPASS_EVT_CAL_END when finished.
      * @return MICROBIT_OK, or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
      *
      * @note *** THIS FUNCITON IS NOW DEPRECATED AND WILL BE REMOVED IN THE NEXT MAJOR RELEASE ***
      * @note *** PLEASE USE THE calibrate() FUNCTION INSTEAD ***
      */
    void calibrateAsync();

    /**
      * Perform a calibration of the compass.
      * This will fire MICROBIT_COMPASS_EVT_CAL_START.
      *
      * @note *** THIS FUNCITON IS NOW DEPRECATED AND WILL BE REMOVED IN THE NEXT MAJOR RELEASE ***
      * @note *** PLEASE USE THE calibrate() FUNCTION INSTEAD ***
      */
    int calibrateStart();

    /**
      * Complete the calibration of the compass.
      * This will fire MICROBIT_COMPASS_EVT_CAL_END.
      *
      * @note *** THIS FUNCITON IS NOW DEPRECATED AND WILL BE REMOVED IN THE NEXT MAJOR RELEASE ***
      */
    void calibrateEnd();

    /**
     * Configure the compass to use the given calibration data.
     * Calibration data is comprised of the perceived zero offset of each axis of the compass.
     * After calibration this should now take into account trimming errors in the magnetometer,
     * and any "hard iron" offsets on the device.
     *
     * @param The x, y and z zero offsets to use as calibration data.
     */
    void setCalibration(CompassSample calibration);

    /**
     * Provides the calibration data currently in use by the compass.
     * More specifically, the x, y and z zero offsets of the compass.
     *
     * @return The x, y and z xero offsets of the compass.
     */
    CompassSample getCalibration();

    /**
      * Periodic callback from MicroBit idle thread.
      * Check if any data is ready for reading by checking the interrupt.
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
      * Returns 0 or 1. 1 indicates data is waiting to be read, zero means data is not ready to be read.
      */
    virtual int isIdleCallbackNeeded();

    /**
      * Destructor for MicroBitCompass, so that we deregister ourselves as an idleComponent
      */
    ~MicroBitCompass();

    private:

    /**
      * Issues a standard, 2 byte I2C command write to the magnetometer.
      * Blocks the calling thread until complete.
      *
      * @param reg The address of the register to write to.
      * @param value The value to write.
      * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
      */
    int writeCommand(uint8_t reg, uint8_t value);

    /**
      * Issues a read command into the specified buffer.
      * Blocks the calling thread until complete.
      *
      * @param reg The address of the register to access.
      * @param buffer Memory area to read the data into.
      * @param length The number of bytes to read.
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
      */
    int readCommand(uint8_t reg, uint8_t* buffer, int length);

    /**
      * Issues a read of a given address, and returns the value.
      * Blocks the calling thread until complete.
      *
      * @param reg The based address of the 16 bit register to access.
      * @return The register value, interpreted as a 16 but signed value, or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
      */
    int read16(uint8_t reg);


    /**
      * Issues a read of a given address, and returns the value.
      * Blocks the calling thread until complete.
      *
      * @param reg The based address of the 16 bit register to access.
      * @return The register value, interpreted as a 8 bit unsigned value, or MICROBIT_I2C_ERROR if the magnetometer could not be accessed.
      */
    int read8(uint8_t reg);
};

#endif
