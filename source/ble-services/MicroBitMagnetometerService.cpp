/**
  * Class definition for the custom MicroBit Magnetometer Service.
  * Provides a BLE service to remotely read the state of the magnetometer, and configure its behaviour.
  */
  
#include "MicroBit.h"
#include "ble/UUID.h"

#include "MicroBitMagnetometerService.h"

/**
  * Constructor. 
  * Create a representation of the MagnetometerService
  * @param _ble The instance of a BLE device that we're running on.
  */
MicroBitMagnetometerService::MicroBitMagnetometerService(BLEDevice &_ble) : 
        ble(_ble) 
{
    // Create the data structures that represent each of our characteristics in Soft Device.
    GattCharacteristic  magnetometerDataCharacteristic(MicroBitMagnetometerServiceDataUUID, (uint8_t *)magnetometerDataCharacteristicBuffer, 0, 
    sizeof(magnetometerDataCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    GattCharacteristic  magnetometerBearingCharacteristic(MicroBitMagnetometerServiceBearingUUID, (uint8_t *)magnetometerBearingCharacteristicBuffer, 0, 
    sizeof(magnetometerBearingCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    GattCharacteristic  magnetometerPeriodCharacteristic(MicroBitMagnetometerServicePeriodUUID, (uint8_t *)magnetometerPeriodCharacteristicBuffer, 0, 
    sizeof(magnetometerPeriodCharacteristicBuffer), 
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE);

    // Initialise our characteristic values.
    magnetometerDataCharacteristicBuffer[0] = 0;
    magnetometerDataCharacteristicBuffer[1] = 0;
    magnetometerDataCharacteristicBuffer[2] = 0;
    magnetometerBearingCharacteristicBuffer = 0;
    magnetometerPeriodCharacteristicBuffer = 0;
    
    GattCharacteristic *characteristics[] = {&magnetometerDataCharacteristic, &magnetometerBearingCharacteristic, &magnetometerPeriodCharacteristic};
    GattService         service(MicroBitMagnetometerServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    magnetometerDataCharacteristicHandle = magnetometerDataCharacteristic.getValueHandle();
    magnetometerBearingCharacteristicHandle = magnetometerBearingCharacteristic.getValueHandle();
    magnetometerPeriodCharacteristicHandle = magnetometerPeriodCharacteristic.getValueHandle();

    ble.updateCharacteristicValue(magnetometerDataCharacteristicHandle, (const uint8_t *)&magnetometerDataCharacteristicBuffer, sizeof(magnetometerDataCharacteristicBuffer));
    ble.updateCharacteristicValue(magnetometerBearingCharacteristicHandle, (const uint8_t *)&magnetometerDataCharacteristicBuffer, sizeof(magnetometerDataCharacteristicBuffer));
    ble.updateCharacteristicValue(magnetometerPeriodCharacteristicHandle, (const uint8_t *)&magnetometerPeriodCharacteristicBuffer, sizeof(magnetometerPeriodCharacteristicBuffer));

    ble.onDataWritten(this, &MicroBitMagnetometerService::onDataWritten);
    uBit.MessageBus.listen(MICROBIT_ID_COMPASS, MICROBIT_COMPASS_EVT_DATA_UPDATE, this, &MicroBitMagnetometerService::magnetometerUpdate, MESSAGE_BUS_LISTENER_NONBLOCKING | MESSAGE_BUS_LISTENER_URGENT);
}

/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitMagnetometerService::onDataWritten(const GattWriteCallbackParams *params)
{   
    if (params->handle == magnetometerPeriodCharacteristicHandle && params->len >= sizeof(magnetometerPeriodCharacteristicBuffer))
    {
        magnetometerPeriodCharacteristicBuffer = *((uint8_t *)params->data);
        uBit.compass.setPeriod(magnetometerPeriodCharacteristicBuffer);
    }
}

/**
  * Magnetometer update callback
  */
void MicroBitMagnetometerService::magnetometerUpdate(MicroBitEvent e)
{
    magnetometerDataCharacteristicBuffer[0] = uBit.compass.getX();
    magnetometerDataCharacteristicBuffer[1] = uBit.compass.getY();
    magnetometerDataCharacteristicBuffer[2] = uBit.compass.getZ();
    magnetometerBearingCharacteristicBuffer = (uint16_t) uBit.compass.heading();

    ble.gattServer().notify(magnetometerDataCharacteristicHandle,(uint8_t *)magnetometerDataCharacteristicBuffer, sizeof(magnetometerDataCharacteristicBuffer));
    ble.gattServer().notify(magnetometerDataCharacteristicHandle,(uint8_t *)magnetometerBearingCharacteristicBuffer, sizeof(magnetometerDataCharacteristicBuffer));
}

const uint8_t  MicroBitMagnetometerServiceUUID[] = {
    0xe9,0x5d,0xf2,0xd8,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitMagnetometerServiceDataUUID[] = {
    0xe9,0x5d,0xfb,0x11,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};


const uint8_t  MicroBitMagnetometerServicePeriodUUID[] = {
    0xe9,0x5d,0x38,0x6c,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

// TODO: Need an assigned UUID from martin here..
const uint8_t  MicroBitMagnetometerServiceBearingUUID[] = {
    0xe9,0x5d,0xde,0xad,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};


