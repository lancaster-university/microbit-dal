/**
  * Class definition for MicroBit Accelerometer.
  *
  * Represents an implementation of the Freescale MMA8653 3 axis accelerometer
  * Also includes basic data caching and on demand activation.
  */
  
#include "MicroBit.h"

/**
  * Configures the accelerometer for G range and sample rate defined
  * in this object. The nearest values are chosen to those defined
  * that are supported by the hardware. The instance variables are then
  * updated to reflect reality.
  */
void MicroBitAccelerometer::configure()
{
    const MMA8653SampleRangeConfig  *actualSampleRange;
    const MMA8653SampleRateConfig  *actualSampleRate;

    // First find the nearest sample rate to that specified.
    actualSampleRate = &MMA8653SampleRate[MMA8653_SAMPLE_RATES-1];
    for (int i=MMA8653_SAMPLE_RATES-1; i>=0; i--)
    {
        if(MMA8653SampleRate[i].sample_period < this->samplePeriod * 1000)
            break;

        actualSampleRate = &MMA8653SampleRate[i];
    }

    // Now find the nearest sample range to that specified.
    actualSampleRange = &MMA8653SampleRange[MMA8653_SAMPLE_RANGES-1];
    for (int i=MMA8653_SAMPLE_RANGES-1; i>=0; i--)
    {
        if(MMA8653SampleRange[i].sample_range < this->sampleRange)
            break;

        actualSampleRange = &MMA8653SampleRange[i];
    }

    // OK, we have the correct data. Update our local state.
    this->samplePeriod = actualSampleRate->sample_period / 1000;
    this->sampleRange = actualSampleRange->sample_range;

    // Now configure the accelerometer accordingly.
    // First place the device into standby mode, so it can be configured.
    writeCommand(MMA8653_CTRL_REG1, 0x00);
    
    // Enable high precisiosn mode. This consumes a bit more power, but still only 184 uA!
    writeCommand(MMA8653_CTRL_REG2, 0x10);

    // Enable the INT1 interrupt pin.
    writeCommand(MMA8653_CTRL_REG4, 0x01);

    // Select the DATA_READY event source to be routed to INT1
    writeCommand(MMA8653_CTRL_REG5, 0x01);

    // Configure for the selected g range.
    writeCommand(MMA8653_XYZ_DATA_CFG, actualSampleRange->xyz_data_cfg);
    
    // Bring the device back online, with 10bit wide samples at the requested frequency.
    writeCommand(MMA8653_CTRL_REG1, actualSampleRate->ctrl_reg1 | 0x01);
}

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

    // Update our internal state for 50Hz at +/- 2g (50Hz has a period af 20ms).
    this->samplePeriod = 20;
    this->sampleRange = 2;

    // Configure and enable the accelerometer.
    this->configure();

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
    static int count=0;

    readCommand(MMA8653_OUT_X_MSB, (uint8_t *)data, 6);

    // read MSB values...
    sample.x = data[0]; 
    sample.y = data[2];
    sample.z = data[4];
   
    // Normalize the data in the 0..1024 range.
    sample.x *= 8;
    sample.y *= 8;
    sample.z *= 8;

    // Invert the x and y axes, so that the reference frame aligns with micro:bit expectations
    sample.x = -sample.x;
    sample.y = -sample.y;

#if CONFIG_ENABLED(USE_ACCEL_LSB)
    // Add in LSB values.
    sample.x += (data[1] / 64);
    sample.y += (data[3] / 64);
    sample.z += (data[5] / 64);
#endif

    // Scale into millig (approx!)
    sample.x *= this->sampleRange;
    sample.y *= this->sampleRange;
    sample.z *= this->sampleRange;

    //TODO: Issue an event.
};

/**
 * Attempts to set the sample rate of the accelerometer to the specified value (in ms).
 * n.b. the requested rate may not be possible on the hardware. In this case, the
 * nearest lower rate is chosen.
 * @param period the requested time between samples, in milliseconds.
 */
void MicroBitAccelerometer::setPeriod(int period)
{
    this->samplePeriod = period;
    this->configure();
}

/**
 * Reads the currently configured sample rate of the accelerometer. 
 * @return The time between samples, in milliseconds.
 */
int MicroBitAccelerometer::getPeriod()
{
    return (int)samplePeriod;
}

/**
 * Attempts to set the sample range of the accelerometer to the specified value (in g).
 * n.b. the requested range may not be possible on the hardware. In this case, the
 * nearest lower rate is chosen.
 * @param range The requested sample range of samples, in g.
 */
void MicroBitAccelerometer::setRange(int range)
{
    this->sampleRange = range;
    this->configure();
}

/**
 * Reads the currently configured sample range of the accelerometer. 
 * @return The sample range, in g.
 */
int MicroBitAccelerometer::getRange()
{
    return (int)sampleRange;
}

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
    //
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

const MMA8653SampleRangeConfig MMA8653SampleRange[MMA8653_SAMPLE_RANGES] = {
    {2, 0}, 
    {4, 1}, 
    {8, 2}
};

const MMA8653SampleRateConfig MMA8653SampleRate[MMA8653_SAMPLE_RATES] = {
    {1250,      0x00},        
    {2500,      0x08}, 
    {5000,      0x10}, 
    {10000,     0x18}, 
    {20000,     0x20}, 
    {80000,     0x28},  
    {160000,    0x30},
    {640000,    0x38}
};
