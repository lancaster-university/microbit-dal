/**
  * Class definition for the custom MicroBit Button Service.
  * Provides a BLE service to remotely read the state of each button, and configure its behaviour.
  */
  
#include "MicroBit.h"
#include "ble/UUID.h"

#include "MicroBitButtonService.h"

/**
  * Constructor. 
  * Create a representation of the ButtonService
  * @param _ble The instance of a BLE device that we're running on.
  */
MicroBitButtonService::MicroBitButtonService(BLEDevice &_ble) : 
        ble(_ble) 
{
    // Create the data structures that represent each of our characteristics in Soft Device.
    GattCharacteristic  buttonADataCharacteristic(MicroBitButtonAServiceDataUUID, (uint8_t *)&buttonADataCharacteristicBuffer, 0, 
    sizeof(buttonADataCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    GattCharacteristic  buttonBDataCharacteristic(MicroBitButtonBServiceDataUUID, (uint8_t *)&buttonADataCharacteristicBuffer, 0, 
    sizeof(buttonADataCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);


    // Initialise our characteristic values.
    buttonADataCharacteristicBuffer = 0;
    buttonBDataCharacteristicBuffer = 0;

    // Set default security requirements
    buttonADataCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    buttonBDataCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    
    GattCharacteristic *characteristics[] = {&buttonADataCharacteristic, &buttonBDataCharacteristic};
    GattService         service(MicroBitButtonServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    buttonADataCharacteristicHandle = buttonADataCharacteristic.getValueHandle();
    buttonBDataCharacteristicHandle = buttonBDataCharacteristic.getValueHandle();

    ble.gattServer().write(buttonADataCharacteristicHandle,(uint8_t *)&buttonADataCharacteristicBuffer, sizeof(buttonADataCharacteristicBuffer));
    ble.gattServer().write(buttonBDataCharacteristicHandle,(uint8_t *)&buttonBDataCharacteristicBuffer, sizeof(buttonBDataCharacteristicBuffer));

    uBit.MessageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_EVT_ANY, this, &MicroBitButtonService::buttonAUpdate, MESSAGE_BUS_LISTENER_IMMEDIATE);
    uBit.MessageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_EVT_ANY, this, &MicroBitButtonService::buttonBUpdate, MESSAGE_BUS_LISTENER_IMMEDIATE);
}


/**
  * Button B update callback
  */
void MicroBitButtonService::buttonAUpdate(MicroBitEvent e)
{
    if (ble.getGapState().connected)
    {
        if (e.value == MICROBIT_BUTTON_EVT_UP)
        {
            buttonADataCharacteristicBuffer = 0;
            ble.gattServer().notify(buttonADataCharacteristicHandle,(uint8_t *)&buttonADataCharacteristicBuffer, sizeof(buttonADataCharacteristicBuffer));
        }

        if (e.value == MICROBIT_BUTTON_EVT_DOWN)
        {
            buttonADataCharacteristicBuffer = 1;
            ble.gattServer().notify(buttonADataCharacteristicHandle,(uint8_t *)&buttonADataCharacteristicBuffer, sizeof(buttonADataCharacteristicBuffer));
        }

        if (e.value == MICROBIT_BUTTON_EVT_HOLD)
        {
            buttonADataCharacteristicBuffer = 2;
            ble.gattServer().notify(buttonADataCharacteristicHandle,(uint8_t *)&buttonADataCharacteristicBuffer, sizeof(buttonADataCharacteristicBuffer));
        }
    }
}

/**
  * Button A update callback
  */
void MicroBitButtonService::buttonBUpdate(MicroBitEvent e)
{
    if (ble.getGapState().connected)
    {
        if (e.value == MICROBIT_BUTTON_EVT_UP)
        {
            buttonBDataCharacteristicBuffer = 0;
            ble.gattServer().notify(buttonBDataCharacteristicHandle,(uint8_t *)&buttonBDataCharacteristicBuffer, sizeof(buttonBDataCharacteristicBuffer));
        }

        if (e.value == MICROBIT_BUTTON_EVT_DOWN)
        {
            buttonBDataCharacteristicBuffer = 1;
            ble.gattServer().notify(buttonBDataCharacteristicHandle,(uint8_t *)&buttonBDataCharacteristicBuffer, sizeof(buttonBDataCharacteristicBuffer));
        }

        if (e.value == MICROBIT_BUTTON_EVT_HOLD)
        {
            buttonBDataCharacteristicBuffer = 2;
            ble.gattServer().notify(buttonBDataCharacteristicHandle,(uint8_t *)&buttonBDataCharacteristicBuffer, sizeof(buttonBDataCharacteristicBuffer));
        }
    }
}

const uint8_t  MicroBitButtonServiceUUID[] = {
    0xe9,0x5d,0x98,0x82,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitButtonAServiceDataUUID[] = {
    0xe9,0x5d,0xda,0x90,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
}
;
const uint8_t  MicroBitButtonBServiceDataUUID[] = {
    0xe9,0x5d,0xda,0x91,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

