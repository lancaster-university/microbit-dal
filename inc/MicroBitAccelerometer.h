#ifndef MICROBIT_ACCELEROMETER_H
#define MICROBIT_ACCELEROMETER_H

#include "mbed.h"

/**
  * Relevant pin assignments
  */
#define MICROBIT_PIN_ACCEL_DATA_READY          P0_28

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

struct MMA8653Sample
{
    int16_t         x;
    int16_t         y;
    int16_t         z;
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
    
    //if you are adding status here - don't it's in MicroBitComponent!!!
    uint16_t        address;       // I2C address of this accelerometer.
    MMA8653Sample   sample;        // The last sample read.
    DigitalIn       int1;          // Data ready interrupt.
    
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
      * Reads the acceleration data from the accelerometer, and stores it in our buffer.
      * This is called by the tick() member function, if the interrupt is set!
      */
    void update();
    
    
    /**
      * Attempts to determine the 8 bit ID from the accelerometer. 
      * @return the 8 bit ID returned by the accelerometer
      *
      * Example:
      * @code 
      * uBit.accelerometer.whoAmI();
      * @endcode
      */
    int whoAmI();

    /**
      * Reads the X axis value of the latest update from the accelerometer.
      * Currently limited to +/- 2g
      * @return The force measured in the X axis, in milli-g.
      *
      * Example:
      * @code 
      * uBit.accelerometer.getX();
      * @endcode
      */
    int getX();
    
    /**
      * Reads the Y axis value of the latest update from the accelerometer.
      * Currently limited to +/- 2g
      * @return The force measured in the Y axis, in milli-g.
      *
      * Example:
      * @code 
      * uBit.accelerometer.getY();
      * @endcode
      */    
    int getY();
    
    /**
      * Reads the Z axis value of the latest update from the accelerometer.
      * Currently limited to +/- 2g
      * @return The force measured in the Z axis, in milli-g.
      *
      * Example:
      * @code 
      * uBit.accelerometer.getZ();
      * @endcode
      */    
    int getZ();

    /**
      * periodic callback from MicroBit idle thread.
      * Check if any data is ready for reading by checking the interrupt flag on the accelerometer
      */    
    virtual void idleTick();
    
    /**
      * Returns 0 or 1. 1 indicates data is waiting to be read, zero means data is not ready to be read.
      */
    virtual int isIdleCallbackNeeded();
    
    private:
    /**
      * Issues a standard, 2 byte I2C command write to the accelerometer.
      * Blocks the calling thread until complete.
      *
      * @param reg The address of the register to write to.
      * @param value The value to write.
      */
    void writeCommand(uint8_t reg, uint8_t value);
    
    /**
      * Issues a read command into the specified buffer.
      * Blocks the calling thread until complete.
      *
      * @param reg The address of the register to access.
      * @param buffer Memory area to read the data into.
      * @param length The number of bytes to read.
      */
    void readCommand(uint8_t reg, uint8_t* buffer, int length);
};

#endif
