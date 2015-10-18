#ifndef MICROBIT_THERMOMETER_H
#define MICROBIT_THERMOMETER_H

#include "mbed.h"
#include "MicroBitComponent.h"

#define MICROBIT_THERMOMETER_PERIOD             1000


#define MAG3110_SAMPLE_RATES                    11 

/*
 * Temperature events
 */
#define MICROBIT_THERMOMETER_EVT_UPDATE         1

/**
  * Class definition for MicroBit Thermometer.
  *
  * Infers and stores the ambient temoperature based on the surface temperature
  * of the various chips on the micro:bit.
  *
  */
class MicroBitThermometer : public MicroBitComponent
{
    unsigned long           sampleTime;
    uint32_t                samplePeriod;
    int16_t                 temperature;              

    public:
    /**
     * Constructor. 
     * Create new object that can sense temperature.
     * @param id the ID of the new MicroBitThermometer object.
     *
     * Example:
     * @code 
     * thermometer(MICROBIT_ID_THERMOMETER); 
     * @endcode
     *
     * Possible Events:
     * @code 
     * MICROBIT_THERMOMETER_EVT_CHANGED
     * @endcode
     */
    MicroBitThermometer(uint16_t id);

    /**
     * Set the sample rate at which the temperatureis read (in ms).
     * n.b. the temperature is alwasy read in the background, so wis only updated
     * when the processor is idle, or when the temperature is explicitly read. 
     * The default sample period is 1 second.
     * @param period the requested time between samples, in milliseconds.
     */
    void setPeriod(int period);

    /**
      * Reads the currently configured sample rate of the thermometer. 
      * @return The time between samples, in milliseconds.
      */
    int getPeriod();

    /**
      * Gets the current temperature of the microbit.
      * @return the current temperature, in degrees celsius.
      *
      * Example:
      * @code
      * uBit.thermometer.getTemperature();
      * @endcode
      */
    int getTemperature();
    
    /**
      * Periodic callback from MicroBit idle thread.
      * Check if any data is ready for reading by checking the interrupt.
      */  
    virtual void idleTick();
    
    /**
      * Indicates if we'd like some processor time to sense the temperature. 0 means we're not due to read the tmeperature yet.
      * @returns 1 if we'd like some processor time, 0 otherwise.
      */
    virtual int isIdleCallbackNeeded();

    private:

    /**
      * Determines if we're due to take another temeoratur reading
      * @return 1 if we're due to take a temperature reading, 0 otherwise.
      */
    int isSampleNeeded();

    /**
      * Updates our recorded temeprature from the many sensors on the micro:bit!
      */
    void updateTemperature();

    
};

#endif
