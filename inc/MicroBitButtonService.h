#ifndef MICROBIT_BUTTON_SERVICE_H
#define MICROBIT_BUTTON_SERVICE_H

#include "MicroBit.h"

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitButtonServiceUUID[];
extern const uint8_t  MicroBitButtonAServiceDataUUID[];
extern const uint8_t  MicroBitButtonBServiceDataUUID[];


/**
  * Class definition for a MicroBit BLE Button Service.
  * Provides access to live button data via BLE, and provides basic configuration options.
  */
class MicroBitButtonService
{                                    
    public:
    
    /**
      * Constructor. 
      * Create a representation of the ButtonService
      * @param _ble The instance of a BLE device that we're running on.
      */
    MicroBitButtonService(BLEDevice &_ble);  
    

    private:

    /**
     * Button A update callback
     */
    void buttonAUpdate(MicroBitEvent e);

    /**
     * Button B update callback
     */
    void buttonBUpdate(MicroBitEvent e);

    // Bluetooth stack we're running on.
    BLEDevice           &ble;

    // memory for our 8 bit control characteristics.
    uint8_t            buttonADataCharacteristicBuffer;
    uint8_t            buttonBDataCharacteristicBuffer;

    // Handles to access each characteristic when they are held by Soft Device.
    GattAttribute::Handle_t buttonADataCharacteristicHandle;
    GattAttribute::Handle_t buttonBDataCharacteristicHandle;
};


#endif

