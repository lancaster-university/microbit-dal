#ifndef MICROBIT_EVENT_SERVICE_H
#define MICROBIT_EVENT_SERVICE_H

#include "MicroBitEvent.h"

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitEventServiceUUID[];
extern const uint8_t  MicroBitEventServiceMicroBitEventCharacteristicUUID[]; 
extern const uint8_t  MicroBitEventServiceClientEventCharacteristicUUID[];
extern const uint8_t  MicroBitEventServiceMicroBitRequirementsCharacteristicUUID[]; 
extern const uint8_t  MicroBitEventServiceClientRequirementsCharacteristicUUID[];

struct EventServiceEvent
{
    uint16_t    type;
    uint16_t    reason;    
};


/**
  * Class definition for a MicroBit BLE Event Service.
  * Provides a _ble gateway onto the MicroBit Message Bus.
  */
class MicroBitEventService : public MicroBitComponent
{                                    
    public:
    
    /**
      * Constructor. 
      * Create a representation of the EventService
      * @param BLE The instance of a BLE device that we're running on.
      */
    MicroBitEventService(BLEDevice &_ble);  
   
    /**
     * Periodic callback from MicroBit scheduler.
     * If we're no longer connected, remove any registered Message Bus listeners.
     */  
    virtual void idleTick();    

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);
    
    /**
      * Callback. Invoked when any events are sent on the microBit message bus.
      */
    void onMicroBitEvent(MicroBitEvent evt);

    /**
     * read callback on microBitRequirements characteristic.
     * Used to iterate through the events that the code on this micro:bit is interested in.
     */  
    void onRequirementsRead(GattReadAuthCallbackParams *params);

    private:

    // Bluetooth stack we're running on.
    BLEDevice           &ble;

    // memory for our event characteristics.
    EventServiceEvent   clientEventBuffer;
    EventServiceEvent   microBitEventBuffer;
    EventServiceEvent   microBitRequirementsBuffer;
    EventServiceEvent   clientRequirementsBuffer;

    // handles on this service's characterisitics.
    GattAttribute::Handle_t microBitEventCharacteristicHandle;
    GattAttribute::Handle_t clientRequirementsCharacteristicHandle;
    GattAttribute::Handle_t clientEventCharacteristicHandle;
    GattCharacteristic *microBitRequirementsCharacteristic;

    // Message bus offset last sent to the client...
    uint16_t messageBusListenerOffset;

};


#endif

