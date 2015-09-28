#ifndef MICROBIT_ACCELEROMETER_SERVICE_H
#define MICROBIT_ACCELEROMETER_SERVICE_H

#include "MicroBit.h"

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitAccelerometerServiceUUID[];
extern const uint8_t  MicroBitAccelerometerServiceDataUUID[];
extern const uint8_t  MicroBitAccelerometerServicePeriodUUID[];


/**
  * Class definition for a MicroBit BLE Accelerometer Service.
  * Provides access to live accelerometer data via BLE, and provides basic configuration options.
  */
class MicroBitAccelerometerService
{                                    
    public:
    
    /**
      * Constructor. 
      * Create a representation of the AccelerometerService
      * @param _ble The instance of a BLE device that we're running on.
      */
    MicroBitAccelerometerService(BLEDevice &_ble);  
    

    private:

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);

    /**
     * Accelerometer update callback
     */
    void accelerometerUpdate(MicroBitEvent e);

    // Bluetooth stack we're running on.
    BLEDevice           &ble;

    // memory for our 8 bit control characteristics.
    uint16_t            accelerometerDataCharacteristicBuffer[3];
    uint16_t            accelerometerPeriodCharacteristicBuffer;

    // Handles to access each characteristic when they are held by Soft Device.
    GattAttribute::Handle_t accelerometerDataCharacteristicHandle;
    GattAttribute::Handle_t accelerometerPeriodCharacteristicHandle;
};


#endif

