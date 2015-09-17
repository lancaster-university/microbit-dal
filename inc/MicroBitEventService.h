#ifndef MICROBIT_EVENT_SERVICE_H
#define MICROBIT_EVENT_SERVICE_H

#include "MicroBitEvent.h"

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitEventServiceUUID[];
extern const uint8_t  MicroBitEventServiceMicroBitEventCharacteristicUUID[]; 
extern const uint8_t  MicroBitEventServiceClientEventCharacteristicUUID[];

struct EventServiceEvent
{
    uint16_t    type;
    uint16_t    reason;    
};


/**
  * Class definition for a MicroBit BLE Event Service.
  * Provides a _ble gateway onto the MicroBit Message Bus.
  */
class MicroBitEventService
{                                    
    public:
    
    /**
      * Constructor. 
      * Create a representation of the EventService
      * @param BLE The instance of a BLE device that we're running on.
      */
    MicroBitEventService(BLEDevice &_ble);  
    
    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);
    
    /**
      * Callback. Invoked when any events are sent on the microBit message bus.
      */
    void onMicroBitEvent(MicroBitEvent evt);

    private:

    // Bluetooth stack we're running on.
    BLEDevice           &ble;

    // memory for our 8 bit control characteristics.
    EventServiceEvent   clientEventBuffer;
    EventServiceEvent   microBitEventBuffer;

    GattAttribute::Handle_t microBitEventCharacteristicHandle;
    GattAttribute::Handle_t clientEventCharacteristicHandle;
};


#endif

