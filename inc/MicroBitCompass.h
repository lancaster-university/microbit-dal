#ifndef MICROBIT_COMPASS_H
#define MICROBIT_COMPASS_H

#include "mbed.h"
#include "MicroBitComponent.h"

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

#define MICROBIT_COMPASS_TEMPERATURE_SENSE_PERIOD 1000

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
#define MICROBIT_COMPASS_EVT_CAL_REQUIRED       1
#define MICROBIT_COMPASS_EVT_CAL_START          2
#define MICROBIT_COMPASS_EVT_CAL_END            3
#define MICROBIT_COMPASS_EVT_DATA_UPDATE        4
#define MICROBIT_COMPASS_EVT_TEMPERATURE_UPDATE 5

/*
 * Status Bits
 */
#define MICROBIT_COMPASS_STATUS_CALIBRATED      1
#define MICROBIT_COMPASS_STATUS_CALIBRATING     2
 
 
#define MICROBIT_COMPASS_CALIBRATE_PERIOD       10000

/*
 * MAG3110 MAGIC ID value
 * Returned from the MAG_WHO_AM_I register for ID purposes.
 */
#define MAG3110_WHOAMI_VAL 0xC4

struct CompassSample
{
    int16_t         x;
    int16_t         y;
    int16_t         z;
    int16_t         temperature;
    
    CompassSample()
    {
        this->x = 0;
        this->y = 0;
        this->z = 0;   
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
    unsigned long       eventStartTime;           // used to store the current system clock when async calibration has started
    unsigned long       temperatureSampleTime;    // used to store the current system clock when async calibration has started
    uint8_t             temperature;              // the current die temperture of the compass chip.

    public:
    
    CompassSample       minSample;      // Calibration sample.
    CompassSample       maxSample;      // Calibration sample.
    CompassSample       average;        // Centre point of sample data.
    CompassSample       sample;         // The latest sample data recorded.
    DigitalIn           int1;           // Data ready interrupt.
            
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
     */
    void configure();

    /**
     * Attempts to set the sample rate of the compass to the specified value (in ms).
     * n.b. the requested rate may not be possible on the hardware. In this case, the
     * nearest lower rate is chosen.
     * @param period the requested time between samples, in milliseconds.
     */
    void setPeriod(int period);

    /**
      * Reads the currently configured sample rate of the compass. 
      * @return The time between samples, in milliseconds.
      */
    int getPeriod();

    /**
      * Gets the current heading of the device, relative to magnetic north.
      * @return the current heading, in degrees.
      *
      * Example:
      * @code
      * uBit.compass.heading();
      * @endcode
      */
    int heading();
    
    /**
      * Attempts to determine the 8 bit ID from the magnetometer. 
      * @return the id of the compass (magnetometer)
      *
      * Example:
      * @code
      * uBit.compass.whoAmI();
      * @endcode
      */
    int whoAmI();

    /**
      * Reads the X axis value of the latest update from the compass.
      * @return The magnetic force measured in the X axis, in no specific units.
      *
      * Example:
      * @code
      * uBit.compass.getX();
      * @endcode
      */
    int getX();
    
    /**
      * Reads the Y axis value of the latest update from the compass.
      * @return The magnetic force measured in the Y axis, in no specific units.
      *
      * Example:
      * @code
      * uBit.compass.getY();
      * @endcode
      */    
    int getY();
    
    /**
      * Reads the Z axis value of the latest update from the compass.
      * @return The magnetic force measured in the Z axis, in no specific units.
      *
      * Example:
      * @code
      * uBit.compass.getZ();
      * @endcode
      */    
    int getZ();    

    /**
      * Reads the currently die temperature of the compass. 
      * @return The temperature, in degrees celsius.
      */
    int getTemperature();

    /**
      * Perform the asynchronous calibration of the compass.
      * This will fire MICROBIT_COMPASS_EVT_CAL_START and MICROBIT_COMPASS_EVT_CAL_END when finished.
      * @note THIS MUST BE CALLED TO GAIN RELIABLE VALUES FROM THE COMPASS
      */
    void calibrateAsync();  

    /**
      * Perform a calibration of the compass.
      * This will fire MICROBIT_COMPASS_EVT_CAL_START.
      * @note THIS MUST BE CALLED TO GAIN RELIABLE VALUES FROM THE COMPASS
      */
    void calibrateStart();    

    /**
      * Complete the calibration of the compass.
      * This will fire MICROBIT_COMPASS_EVT_CAL_END.
      * @note THIS MUST BE CALLED TO GAIN RELIABLE VALUES FROM THE COMPASS
      */    
    void calibrateEnd();    

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
    
    private:
    
    /**
      * Issues a standard, 2 byte I2C command write to the magnetometer.
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
    
    /**
      * Issues a read of a given address, and returns the value.
      * Blocks the calling thread until complete.
      *
      * @param reg The based address of the 16 bit register to access.
      * @return The register value, interpreted as a 16 but signed value.
      */
    int16_t read16(uint8_t reg);
    
    
    /**
      * Issues a read of a given address, and returns the value.
      * Blocks the calling thread until complete.
      *
      * @param reg The based address of the 16 bit register to access.
      * @return The register value, interpreted as a 8 bi signed value.
      */
    int16_t read8(uint8_t reg);
};

#endif
