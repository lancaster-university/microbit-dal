/**
  * Class definition for a MicroBit BLE Event Service.
  * Provides a BLE gateway onto the MicroBit Message Bus.
  */
  
#include "MicroBit.h"
#include "ble/UUID.h"

#include "ExternalEvents.h"
#include "MicroBitEventService.h"

/**
  * Constructor. 
  * Create a representation of the EventService
  * @param _ble The instance of a BLE device that we're running on.
  */
MicroBitEventService::MicroBitEventService(BLEDevice &_ble) : 
        ble(_ble) 
{
    GattCharacteristic  microBitEventCharacteristic(MicroBitEventServiceMicroBitEventCharacteristicUUID, (uint8_t *)&microBitEventBuffer, 0, sizeof(EventServiceEvent), 
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    GattCharacteristic  clientEventCharacteristic(MicroBitEventServiceClientEventCharacteristicUUID, (uint8_t *)&clientEventBuffer, 0, sizeof(EventServiceEvent),
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);

    clientEventBuffer.type = 0x00;
    clientEventBuffer.reason = 0x00;
    
    microBitEventBuffer.type = 0x00;
    microBitEventBuffer.reason = 0x00;
    
    GattCharacteristic *characteristics[] = {&microBitEventCharacteristic, &clientEventCharacteristic};
    GattService         service(MicroBitEventServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    microBitEventCharacteristicHandle = microBitEventCharacteristic.getValueHandle();
    clientEventCharacteristicHandle = clientEventCharacteristic.getValueHandle();

    ble.onDataWritten(this, &MicroBitEventService::onDataWritten);
}


/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitEventService::onDataWritten(const GattWriteCallbackParams *params)
{   
    int len = params->len; 
    EventServiceEvent *e = (EventServiceEvent *)params->data;
    
    if (params->handle == clientEventCharacteristicHandle) {
    
        // Read and fire all events...
        while (len >= 4)
        {
            MicroBitEvent evt(e->type, e->reason);
            len-=4;
            e++;
        }
    }
}

/**
  * Callback. Invoked when any events are sent on the microBit message bus.
  */
void MicroBitEventService::onMicroBitEvent(MicroBitEvent evt)
{
    EventServiceEvent *e = &microBitEventBuffer;
    
    if (ble.getGapState().connected) {
        e->type = evt.source;
        e->reason = evt.value;
        
        ble.updateCharacteristicValue(microBitEventCharacteristicHandle, (const uint8_t *)e, sizeof(EventServiceEvent));
    }
}

const uint8_t              MicroBitEventServiceUUID[] = {
    0xe9,0x5d,0x93,0xaf,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t              MicroBitEventServiceMicroBitEventCharacteristicUUID[] = {
    0xe9,0x5d,0x97,0x75,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t              MicroBitEventServiceClientEventCharacteristicUUID[] = {
    0xe9,0x5d,0x54,0x04,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

