#ifndef MICROBIT_THERMOMETER_H
#define MICROBIT_THERMOMETER_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "MicroBitStorage.h"

#define MICROBIT_THERMOMETER_PERIOD             1000


#define MAG3110_SAMPLE_RATES                    11

/*
 * Temperature events
 */
#define MICROBIT_THERMOMETER_EVT_UPDATE         1

#define MICROBIT_THERMOMETER_ADDED_TO_IDLE      2

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
    int16_t                 offset;
    MicroBitStorage*        storage;

    public:

    /**
      * Constructor.
      * Create new object that can sense temperature.
      * @param id the ID of the new MicroBitThermometer object.
      * @param _storage an instance of MicroBitStorage used to persist temperature offset data
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
    MicroBitThermometer(uint16_t id, MicroBitStorage& _storage);

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
      * Set the value that is used to offset the raw silicon temperature.
      *
      * @param offset the offset for the silicon temperature
      *
      * @return MICROBIT_OK on success
      */
    int setOffset(int offset);

    /**
      * Retreive the value that is used to offset the raw silicon temperature.
      *
      * @return
      */
    int getOffset();

    /**
      * This member function fetches the raw silicon temperature, and calculates
      * the value used to offset the raw silicon temperature based on a given temperature.
      *
      * @param calibrationTemp the temperature used to calculate the raw silicon temperature
      * offset.
      *
      * @return MICROBIT_OK on success
      */
    int setCalibration(int calibrationTemp);

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
      * Updates the temperature sample of this instance of MicroBitThermometer,
      * only if isSampleNeeded() indicates that an update is required.
      * This call also will add the thermometer to fiber components to receive
      * periodic callbacks.
      *
      * @return MICROBIT_OK on success.
      */
    int updateSample();

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
};

#endif
