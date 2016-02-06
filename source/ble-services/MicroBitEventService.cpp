/**
  * Class definition for a MicroBit BLE Event Service.
  * Provides a BLE gateway onto the MicroBit Message Bus.
  */
  
#include "MicroBit.h"
#include "ble/UUID.h"
#include "ExternalEvents.h"

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

    GattCharacteristic  clientRequirementsCharacteristic(MicroBitEventServiceClientRequirementsCharacteristicUUID, (uint8_t *)&clientRequirementsBuffer, 0, sizeof(EventServiceEvent), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);

    microBitRequirementsCharacteristic = new GattCharacteristic(MicroBitEventServiceMicroBitRequirementsCharacteristicUUID, (uint8_t *)&microBitRequirementsBuffer, 0, sizeof(EventServiceEvent), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    microBitRequirementsCharacteristic->setReadAuthorizationCallback(this, &MicroBitEventService::onRequirementsRead);

    clientEventBuffer.type = 0x00;
    clientEventBuffer.reason = 0x00;
    
    microBitEventBuffer = microBitRequirementsBuffer = clientRequirementsBuffer = clientEventBuffer;

    messageBusListenerOffset = 0;
    
    // Set default security requirements
    microBitEventCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    clientEventCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    clientRequirementsCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    microBitRequirementsCharacteristic->requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);

    GattCharacteristic *characteristics[] = {&microBitEventCharacteristic, &clientEventCharacteristic, &clientRequirementsCharacteristic, microBitRequirementsCharacteristic};
    GattService         service(MicroBitEventServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    microBitEventCharacteristicHandle = microBitEventCharacteristic.getValueHandle();
    clientEventCharacteristicHandle = clientEventCharacteristic.getValueHandle();
    clientRequirementsCharacteristicHandle = clientRequirementsCharacteristic.getValueHandle();

    ble.onDataWritten(this, &MicroBitEventService::onDataWritten);

    uBit.addIdleComponent(this);
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
        return;
    }

    if (params->handle == clientRequirementsCharacteristicHandle) {
        // Read and register for all the events given...
        while (len >= 4)
        {
            uBit.MessageBus.listen(e->type, e->reason, this, &MicroBitEventService::onMicroBitEvent, MESSAGE_BUS_LISTENER_IMMEDIATE);

            len-=4;
            e++;
        }
        return;
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
        
        ble.gattServer().notify(microBitEventCharacteristicHandle, (const uint8_t *)e, sizeof(EventServiceEvent));
    } 
}

/**
 * Periodic callback from MicroBit scheduler.
 * If we're no longer connected, remove any registered Message Bus listeners.
 */  
void MicroBitEventService::idleTick()
{
    if (!ble.getGapState().connected && messageBusListenerOffset >0) {
        messageBusListenerOffset = 0;  
        uBit.MessageBus.ignore(MICROBIT_ID_ANY, MICROBIT_EVT_ANY, this, &MicroBitEventService::onMicroBitEvent);
    }
}

/**
 * read callback on data characteristic.
 * reads all the pins marked as inputs, and updates the data stored in the BLE stack.
 */  
void MicroBitEventService::onRequirementsRead(GattReadAuthCallbackParams *params)
{
    if (params->handle == microBitRequirementsCharacteristic->getValueHandle())
    {
        // Walk through the lsit of message bus listeners.
        // We send one at a time, and our client can keep reading from this characterisitic until we return an emtpy value.
        MicroBitListener *l = uBit.MessageBus.elementAt(messageBusListenerOffset++); 

        if (l != NULL)
        {
            microBitRequirementsBuffer.type = l->id;
            microBitRequirementsBuffer.reason = l->value;
            ble.gattServer().write(microBitRequirementsCharacteristic->getValueHandle(), (uint8_t *)&microBitRequirementsBuffer, sizeof(EventServiceEvent));
        } else {
            ble.gattServer().write(microBitRequirementsCharacteristic->getValueHandle(), (uint8_t *)&microBitRequirementsBuffer, 0);
        }
    }
}

const uint8_t  MicroBitEventServiceUUID[] = {
    0xe9,0x5d,0x93,0xaf,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitEventServiceMicroBitEventCharacteristicUUID[] = {
    0xe9,0x5d,0x97,0x75,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitEventServiceClientEventCharacteristicUUID[] = {
    0xe9,0x5d,0x54,0x04,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitEventServiceMicroBitRequirementsCharacteristicUUID[] = {
    0xe9,0x5d,0xb8,0x4c,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitEventServiceClientRequirementsCharacteristicUUID[] = {
    0xe9,0x5d,0x23,0xc4,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

