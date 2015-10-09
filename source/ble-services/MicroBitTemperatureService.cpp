/**
  * Class definition for the custom MicroBit Temperature Service.
  * Provides a BLE service to remotely read the state of the temperature, and configure its behaviour.
  */
  
#include "MicroBit.h"
#include "ble/UUID.h"

#include "MicroBitTemperatureService.h"

/**
  * Constructor. 
  * Create a representation of the TemperatureService
  * @param _ble The instance of a BLE device that we're running on.
  */
MicroBitTemperatureService::MicroBitTemperatureService(BLEDevice &_ble) : 
        ble(_ble) 
{
    // Create the data structures that represent each of our characteristics in Soft Device.
    GattCharacteristic  temperatureDataCharacteristic(MicroBitTemperatureServiceDataUUID, (uint8_t *)&temperatureDataCharacteristicBuffer, 0, 
    sizeof(temperatureDataCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    // Initialise our characteristic values.
    temperatureDataCharacteristicBuffer = 0;
    
    GattCharacteristic *characteristics[] = {&temperatureDataCharacteristic};
    GattService         service(MicroBitTemperatureServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    temperatureDataCharacteristicHandle = temperatureDataCharacteristic.getValueHandle();
    ble.gattServer().write(temperatureDataCharacteristicHandle,(uint8_t *)&temperatureDataCharacteristicBuffer, sizeof(temperatureDataCharacteristicBuffer));

    uBit.MessageBus.listen(MICROBIT_ID_THERMOMETER, MICROBIT_THERMOMETER_EVT_UPDATE, this, &MicroBitTemperatureService::temperatureUpdate, MESSAGE_BUS_LISTENER_IMMEDIATE);
}

/**
  * Temperature update callback
  */
void MicroBitTemperatureService::temperatureUpdate(MicroBitEvent e)
{
    if (ble.getGapState().connected)
    {
        temperatureDataCharacteristicBuffer = uBit.thermometer.getTemperature();
        ble.gattServer().notify(temperatureDataCharacteristicHandle,(uint8_t *)&temperatureDataCharacteristicBuffer, sizeof(temperatureDataCharacteristicBuffer));
    }
}

const uint8_t  MicroBitTemperatureServiceUUID[] = {
    0xe9,0x5d,0x61,0x00,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitTemperatureServiceDataUUID[] = {
    0xe9,0x5d,0x92,0x50,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};


