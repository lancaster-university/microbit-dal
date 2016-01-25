#include "MicroBit.h"

/**
  * Turn off warnings under gcc -Wall. We turn off unused-function for the
  * entire compilation unit as the compiler can't tell if a function is
  * unused until the end of the unit.  The macro expansion for SVCALL()
  * in nrf_soc.h and nrf_srv.h tries to leave unused-function turned off,
  * but we restore the state from before the include with our diagnostics
  * pop.
  * It might be cleaner to add
  * #pragram GCC system header
  * as the first line of nrf_soc.h, but that's a different
  * module ...
  */


/* 
 * The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
#if !defined(__arm)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "nrf_soc.h"

/* 
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif

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
MicroBitThermometer::MicroBitThermometer(uint16_t id)
{
    this->id = id;
    this->samplePeriod = MICROBIT_THERMOMETER_PERIOD;
    this->sampleTime = 0;

    uBit.addIdleComponent(this);
}

/**
 * Gets the current temperature of the microbit.
 * @return the current temperature, in degrees celsius.
 *
 * Example:
 * @code
 * uBit.thermometer.getTemperature();
 * @endcode
 */
int MicroBitThermometer::getTemperature()
{
    if (isSampleNeeded())
        updateTemperature();

    return temperature;
}

/**
 * Indicates if we'd like some processor time to sense the temperature. 0 means we're not due to read the tmeperature yet.
 * @returns 1 if we'd like some processor time, 0 otherwise.
 */
int MicroBitThermometer::isIdleCallbackNeeded()
{
    return isSampleNeeded();    
}
/**
  * periodic callback.
  * Check once every second or so for a new temperature reading.
  */  
void MicroBitThermometer::idleTick()
{   
    if (isSampleNeeded())
        updateTemperature();
}

/**
 * Determines if we're due to take another temeoratur reading
 * @return 1 if we're due to take a temperature reading, 0 otherwise.
 */
int MicroBitThermometer::isSampleNeeded()
{ 
    return  ticks >= sampleTime;
}

/**
 * Set the sample rate at which the temperatureis read (in ms).
 * n.b. the temperature is alwasy read in the background, so wis only updated
 * when the processor is idle, or when the temperature is explicitly read. 
 * The default sample period is 1 second.
 * @param period the requested time between samples, in milliseconds.
 */
void MicroBitThermometer::setPeriod(int period)
{
    samplePeriod = period; 
}

/**
 * Reads the currently configured sample rate of the thermometer. 
 * @return The time between samples, in milliseconds.
 */
int MicroBitThermometer::getPeriod()
{
    return samplePeriod;
}

/**
 * Updates our recorded temperature from the many sensors on the micro:bit!
 */
void MicroBitThermometer::updateTemperature()
{
    int32_t processorTemperature;

    // For now, we just rely on the nrf senesor to be the most accurate.
    // The compass module also has a temperature sensor, and has the lowest power consumption, so will run the cooler...
    // ...however it isn't trimmed for accuracy during manufacture, so requires calibration.

    if (uBit.ble)
    {
        // If Bluetooth is enabled, we need to go through the Nordic software to safely do this
        sd_temp_get(&processorTemperature);
    }
    else
    {
        // Othwerwise, we access the information directly...
        uint32_t *TEMP = (uint32_t *)0x4000C508;

        NRF_TEMP->TASKS_START = 1;

        while (NRF_TEMP->EVENTS_DATARDY == 0);

        NRF_TEMP->EVENTS_DATARDY = 0;  

        processorTemperature = *TEMP;

        NRF_TEMP->TASKS_STOP = 1;
    }


    // Record our reading...
    temperature = processorTemperature / 4;

    // Schedule our next sample.
    sampleTime = ticks + samplePeriod;
    
    // Send an event to indicate that we'e updated our temperature.
    MicroBitEvent e(id, MICROBIT_THERMOMETER_EVT_UPDATE);
}


