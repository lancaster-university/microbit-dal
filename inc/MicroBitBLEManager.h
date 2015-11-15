#ifndef MICROBIT_BLE_MANAGER_H
#define MICROBIT_BLE_MANAGER_H

#include "mbed.h"

/* 
 * The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
#if !defined (__arm)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "ble/BLE.h"

/* 
 * Return to our predefined compiler settings.
 */
#if !defined (__arm)
#pragma GCC diagnostic pop
#endif

#include "ble/services/DeviceInformationService.h"
#include "MicroBitDFUService.h"
#include "MicroBitEventService.h"
#include "MicroBitLEDService.h"
#include "MicroBitAccelerometerService.h"
#include "MicroBitMagnetometerService.h"
#include "MicroBitButtonService.h"
#include "MicroBitIOPinService.h"
#include "MicroBitTemperatureService.h"
#include "ExternalEvents.h"

/**
  * Class definition for the MicroBitBLEManager.
  *
  */
class MicroBitBLEManager
{                                  
    public:

	// The mbed abstraction of the BlueTooth Low Energy (BLE) hardware
    BLEDevice                       *ble;
    
    /**
     * Constructor. 
     *
     * Configure and manage the micro:bit's Bluetooth Low Energy (BLE) stack.
     * Note that the BLE stack *cannot*  be brought up in a static context.
     * (the software simply hangs or corrupts itself).
     * Hence, we bring it up in an explicit init() method, rather than in the constructor.
     */
    MicroBitBLEManager();  

    /**
      * Post constructor initialisation method.
      * After *MUCH* pain, it's noted that the BLE stack can't be brought up in a 
      * static context, so we bring it up here rather than in the constructor.
      * n.b. This method *must* be called in main() or later, not before.
      *
      * Example:
      * @code 
      * uBit.init();
      * @endcode
      */
    void init(ManagedString deviceName, ManagedString serialNumber);

    /**
     * Enter BLUEZOE mode. This is mode is called to initiate pairing, and to enable FOTA programming
     * of the micro:bit in cases where BLE is disabled during normal operation.
     *
     * @param display a MicroBitDisplay to use when displaying pairing information.
     */
    void bluezone(MicroBitDisplay &display);

    /**
     * Method that is called whenever a BLE device disconnects from us.
     * The nordic stack stops dvertising whenever a device connects, so we use
     * this callback to restart advertising.
     */
    void onDisconnectionCallback();
};


#endif

