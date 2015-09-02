/**
  * Class definition for MicroBit Accelerometer.
  *
  * Represents an implementation of the Freescale MMA8653 3 axis accelerometer
  * Also includes basic data caching and on demand activation.
  */
  
#include "MicroBit.h"

/**
  * Issues a standard, 2 byte I2C command write to the accelerometer.
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to write to.
  * @param value The value to write.
  */
void MicroBitAccelerometer::writeCommand(uint8_t reg, uint8_t value)
{
    uint8_t command[2];
    command[0] = reg;
    command[1] = value;
    
    uBit.i2c.write(address, (const char *)command, 2);
}

/**
  * Issues a read command into the specified buffer.
  * Blocks the calling thread until complete.
  *
  * @param reg The address of the register to access.
  * @param buffer Memory area to read the data into.
  * @param length The number of bytes to read.
  */
void MicroBitAccelerometer::readCommand(uint8_t reg, uint8_t* buffer, int length)
{
    uBit.i2c.write(address, (const char *)&reg, 1, true);
    uBit.i2c.read(address, (char *)buffer, length);
}

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
MicroBitAccelerometer::MicroBitAccelerometer(uint16_t id, uint16_t address) : sample(), int1(MICROBIT_PIN_ACCEL_DATA_READY)
{
    // Store our identifiers.
    this->id = id;
    this->address = address;

    // Enable the accelerometer.
    // First place the device into standby mode, so it can be configured.
    writeCommand(MMA8653_CTRL_REG1, 0x00);
    
    // Enable the INT1 interrupt pin.
    writeCommand(MMA8653_CTRL_REG4, 0x01);

    // Select the DATA_READY event source to be routed to INT1
    writeCommand(MMA8653_CTRL_REG5, 0x01);

    // Configure for a +/- 2g range.
    writeCommand(MMA8653_XYZ_DATA_CFG, 0x00);
    
    // Bring the device back online, with 10bit wide samples at a 50Hz frequency.
    writeCommand(MMA8653_CTRL_REG1, 0x21);

    // indicate that we're ready to receive tick callbacks.
    uBit.flags |= MICROBIT_FLAG_ACCELEROMETER_RUNNING;
}

/**
  * Attempts to determine the 8 bit ID from the accelerometer. 
  * @return the 8 bit ID returned by the accelerometer
  *
  * Example:
  * @code 
  * uBit.accelerometer.whoAmI();
  * @endcode
  */
int MicroBitAccelerometer::whoAmI()
{
    uint8_t data;

    readCommand(MMA8653_WHOAMI, &data, 1);    
    return (int)data;
}

/**
  * Reads the acceleration data from the accelerometer, and stores it in our buffer.
  * This is called by the tick() member function, if the interrupt is set!
  */
void MicroBitAccelerometer::update()
{
    int8_t data[6];

    readCommand(MMA8653_OUT_X_MSB, (uint8_t *)data, 6);

    // read MSB values...
    sample.x = data[0]; 
    sample.y = data[2];
    sample.z = data[4];
    
    // Scale into millig (approx!)
    sample.x *= 16;
    sample.y *= 16;
    sample.z *= 16;

    // Invert the x and y axes, so that the reference frame aligns with micro:bit expectations
    sample.x = -sample.x;
    sample.y = -sample.y;

    // We ignore the LSB bits for now, as they're just noise...
    // TODO: Revist this when we have working samples to see if additional resolution is needed.

#if CONFIG_ENABLED(USE_ACCEL_LSB)
    // Add in LSB values.
    sample.x += (data[1] / 64);
    sample.y += (data[3] / 64);
    sample.z += (data[5] / 64);
#endif

    //TODO: Issue an event.
};

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
int MicroBitAccelerometer::getX()
{
    return sample.x;
}

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
int MicroBitAccelerometer::getY()
{
    return sample.y;
}

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
int MicroBitAccelerometer::getZ()
{
    return sample.z;
}


/**
  * periodic callback from MicroBit clock.
  * Check if any data is ready for reading by checking the interrupt flag on the accelerometer
  */  
void MicroBitAccelerometer::idleTick()
{
    // Poll interrupt line from accelerometer.
    // n.b. Default is Active LO. Interrupt is cleared in data read.
    if(!int1)
        update();
}

/**
  * Returns 0 or 1. 1 indicates data is waiting to be read, zero means data is not ready to be read.
  */
int MicroBitAccelerometer::isIdleCallbackNeeded()
{
    return !int1;
}
