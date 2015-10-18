#ifndef MICROBIT_MAGNETOMETER_SERVICE_H
#define MICROBIT_MAGNETOMETER_SERVICE_H

#include "MicroBit.h"

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitMagnetometerServiceUUID[];
extern const uint8_t  MicroBitMagnetometerServiceDataUUID[];
extern const uint8_t  MicroBitMagnetometerServiceBearingUUID[];
extern const uint8_t  MicroBitMagnetometerServicePeriodUUID[];


/**
  * Class definition for a MicroBit BLE Magnetometer Service.
  * Provides access to live magnetometer data via BLE, and provides basic configuration options.
  */
class MicroBitMagnetometerService
{                                    
    public:
    
    /**
      * Constructor. 
      * Create a representation of the MagnetometerService
      * @param _ble The instance of a BLE device that we're running on.
      */
    MicroBitMagnetometerService(BLEDevice &_ble);  
    

    private:

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);

    /**
     * Magnetometer update callback
     */
    void magnetometerUpdate(MicroBitEvent e);

    /**
     * Sample Period Change Needed callback.
     * Reconfiguring the magnetometer can to a REALLY long time (sometimes even seconds to complete)
     * So we do this in the background when necessary, through this event handler. 
     */
    void samplePeriodUpdateNeeded(MicroBitEvent e);

    // Bluetooth stack we're running on.
    BLEDevice           &ble;

    // memory for our 8 bit control characteristics.
    int16_t             magnetometerDataCharacteristicBuffer[3];
    uint16_t            magnetometerBearingCharacteristicBuffer;
    uint16_t            magnetometerPeriodCharacteristicBuffer;

    // Handles to access each characteristic when they are held by Soft Device.
    GattAttribute::Handle_t magnetometerDataCharacteristicHandle;
    GattAttribute::Handle_t magnetometerBearingCharacteristicHandle;
    GattAttribute::Handle_t magnetometerPeriodCharacteristicHandle;
};

#endif

