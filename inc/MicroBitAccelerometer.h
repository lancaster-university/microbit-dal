#ifndef MICROBIT_ACCELEROMETER_H
#define MICROBIT_ACCELEROMETER_H

#include "mbed.h"
#include "MicroBitComponent.h"
#include "MicroBitCoordinateSystem.h"

/**
 * Relevant pin assignments
 */
#define MICROBIT_PIN_ACCEL_DATA_READY          P0_28

/**
  * Status flags
  */
#define MICROBIT_ACCEL_PITCH_ROLL_VALID           0x01

/*
 * I2C constants
 */
#define MMA8653_DEFAULT_ADDR    0x3A

/*
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

/*
 * Accelerometer events
 */
#define MICROBIT_ACCELEROMETER_EVT_DATA_UPDATE              1

/*
 * Gesture events
 */
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

/*
 * Gesture recogniser constants
 */
#define MICROBIT_ACCELEROMETER_REST_TOLERANCE               200
#define MICROBIT_ACCELEROMETER_TILT_TOLERANCE               200
#define MICROBIT_ACCELEROMETER_FREEFALL_TOLERANCE           400
#define MICROBIT_ACCELEROMETER_SHAKE_TOLERANCE              1000
#define MICROBIT_ACCELEROMETER_3G_TOLERANCE                 3072
#define MICROBIT_ACCELEROMETER_6G_TOLERANCE                 6144
#define MICROBIT_ACCELEROMETER_8G_TOLERANCE                 8192
#define MICROBIT_ACCELEROMETER_GESTURE_DAMPING              10
#define MICROBIT_ACCELEROMETER_SHAKE_DAMPING                10

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

enum BasicGesture
{
    GESTURE_NONE,
    GESTURE_UP,
    GESTURE_DOWN,
    GESTURE_LEFT,
    GESTURE_RIGHT,
    GESTURE_FACE_UP,
    GESTURE_FACE_DOWN,
    GESTURE_FREEFALL,
    GESTURE_3G,
    GESTURE_6G,
    GESTURE_8G,
    GESTURE_SHAKE
};

struct ShakeHistory
{
    uint16_t    shaken:1,
                x:1,
                y:1,
                z:1,
                count:4,
                timer:8;
};

/**
 * Class definition for MicroBit Accelerometer.
 *
 * Represents an implementation of the Freescale MMA8653 3 axis accelerometer
 * Also includes basic data caching and on demand activation.
 */
class MicroBitAccelerometer : public MicroBitComponent
{
    /**
      * Unique, enumerated ID for this component.
      * Used to track asynchronous events in the event bus.
      */

    uint16_t        address;            // I2C address of this accelerometer.
    uint16_t        samplePeriod;       // The time between samples, in milliseconds.
    uint8_t         sampleRange;        // The sample range of the accelerometer in g.
    MMA8653Sample   sample;             // The last sample read.
    DigitalIn       int1;               // Data ready interrupt.
    float           pitch;              // Pitch of the device, in radians.
    float           roll;               // Roll of the device, in radians.
    uint8_t         sigma;              // the number of ticks that the instantaneous gesture has been stable.
    BasicGesture    lastGesture;        // the last, stable gesture recorded.
    BasicGesture    currentGesture;     // the instantaneous, unfiltered gesture detected.
    ShakeHistory    shake;              // State information needed to detect shake events.

    public:

    /**
     * Constructor.
     * Create an accelerometer representation with the given ID.
     * @param id the ID of the new object.
     * @param address the default base address of the accelerometer.
     *
     * Example:
     * @code
     * accelerometer(MICROBIT_ID_ACCELEROMETER, MMA8653_DEFAULT_ADDR)
     * @endcode
     */
    MicroBitAccelerometer(uint16_t id, uint16_t address);

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
     * This is called by the tick() member function, if the interrupt is set.
     *
     * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the read request fails.
     */
    int update();

    /**
     * Attempts to set the sample rate of the accelerometer to the specified value (in ms).
     * n.b. the requested rate may not be possible on the hardware. In this case, the
     * nearest lower rate is chosen.
     * @param period the requested time between samples, in milliseconds.
     * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
     */
    int setPeriod(int period);

    /**
     * Reads the currently configured sample rate of the accelerometer.
     * @return The time between samples, in milliseconds.
     */
    int getPeriod();

    /**
     * Attempts to set the sample range of the accelerometer to the specified value (in g).
     * n.b. the requested range may not be possible on the hardware. In this case, the
     * nearest lower rate is chosen.
     * @param range The requested sample range of samples, in g.
     * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR is the request fails.
     */
    int setRange(int range);

    /**
     * Reads the currently configured sample range of the accelerometer.
     * @return The sample range, in g.
     */
    int getRange();

    /**
     * Attempts to determine the 8 bit ID from the accelerometer.
     * @return the 8 bit ID returned by the accelerometer, or MICROBIT_I2C_ERROR if the request fails.
     *
     * Example:
     * @code
     * uBit.accelerometer.whoAmI();
     * @endcode
     */
    int whoAmI();

    /**
      * Reads the X axis value of the latest update from the accelerometer.
      * @param system The coordinate system to use. By default, a simple cartesian system is provided.
      * @return The force measured in the X axis, in milli-g.
      *
      * Example:
      * @code
      * uBit.accelerometer.getX();
      * @endcode
      */
    int getX(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
      * Reads the Y axis value of the latest update from the accelerometer.
      * @return The force measured in the Y axis, in milli-g.
      *
      * Example:
      * @code
      * uBit.accelerometer.getY();
      * @endcode
      */
    int getY(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
      * Reads the Z axis value of the latest update from the accelerometer.
      * @return The force measured in the Z axis, in milli-g.
      *
      * Example:
      * @code
      * uBit.accelerometer.getZ();
      * @endcode
      */
    int getZ(MicroBitCoordinateSystem system = SIMPLE_CARTESIAN);

    /**
      * Provides a rotation compensated pitch of the device, based on the latest update from the accelerometer.
      * @return The pitch of the device, in degrees.
      *
      * Example:
      * @code
      * uBit.accelerometer.getPitch();
      * @endcode
      */
    int getPitch();
    float getPitchRadians();

    /**
      * Provides a rotation compensated roll of the device, based on the latest update from the accelerometer.
      * @return The roll of the device, in degrees.
      *
      * Example:
      * @code
      * uBit.accelerometer.getRoll();
      * @endcode
      */
    int getRoll();
    float getRollRadians();

    /**
     * Reads the last recorded gesture detected.
     * @return The last gesture detected.
     *
     * Example:
     * @code
     * if (uBit.accelerometer.getGesture() == SHAKE)
     * @endcode
     */
    BasicGesture getGesture();

    /**
     * periodic callback from MicroBit idle thread.
     * Check if any data is ready for reading by checking the interrupt flag on the accelerometer
     */
    virtual void idleTick();

    /**
     * Returns 0 or 1. 1 indicates data is waiting to be read, zero means data is not ready to be read.
     */
    virtual int isIdleCallbackNeeded();

    /**
      * Destructor for MicroBitButton, so that we deregister ourselves as an idleComponent
      */
    ~MicroBitAccelerometer();

    private:
    /**
     * Issues a standard, 2 byte I2C command write to the accelerometer.
     * Blocks the calling thread until complete.
     *
     * @param reg The address of the register to write to.
     * @param value The value to write.
     * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the the write request failed.
     */
    int writeCommand(uint8_t reg, uint8_t value);

    /**
     * Issues a read command into the specified buffer.
     * Blocks the calling thread until complete.
     *
     * @param reg The address of the register to access.
     * @param buffer Memory area to read the data into.
     * @param length The number of bytes to read.
     * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER or MICROBIT_I2C_ERROR if the the read request failed.
     */
    int readCommand(uint8_t reg, uint8_t* buffer, int length);

    /**
     * Recalculate roll and pitch values for the current sample.
     * We only do this at most once per sample, as the necessary trigonemteric functions are rather
     * heavyweight for a CPU without a floating point unit...
     */
    void recalculatePitchRoll();

    /**
     *
     * Updates the basic gesture recognizer. This performs instantaneous pose recognition, and also some low pass filtering to promote
     * stability.
     */
    void updateGesture();

    /**
     * Service function. Calculates the current scalar acceleration of the device (x^2 + y^2 + z^2).
     * It does not, however, square root the result, as this is a relatively high cost operation.
     * This is left to application code should it be needed.
     * @return the sum of the square of the acceleration of the device across all axes.
     */
    int instantaneousAccelerationSquared();

    /**
     * Service function. Determines the best guess posture of the device based on instantaneous data.
     * This makes no use of historic data, and forms this input to th filter implemented in updateGesture().
     * @return A best guess of the curret posture of the device, based on instanataneous data.
     */
    BasicGesture instantaneousPosture();
};

#endif
