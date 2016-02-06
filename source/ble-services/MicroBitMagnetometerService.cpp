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

    GattCharacteristic  magnetometerBearingCharacteristic(MicroBitMagnetometerServiceBearingUUID, (uint8_t *)&magnetometerBearingCharacteristicBuffer, 0, 
    sizeof(magnetometerBearingCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    GattCharacteristic  magnetometerPeriodCharacteristic(MicroBitMagnetometerServicePeriodUUID, (uint8_t *)&magnetometerPeriodCharacteristicBuffer, 0, 
    sizeof(magnetometerPeriodCharacteristicBuffer), 
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);

    // Initialise our characteristic values.
    magnetometerDataCharacteristicBuffer[0] = 0;
    magnetometerDataCharacteristicBuffer[1] = 0;
    magnetometerDataCharacteristicBuffer[2] = 0;
    magnetometerBearingCharacteristicBuffer = 0;
    magnetometerPeriodCharacteristicBuffer = uBit.compass.getPeriod();

    // Set default security requirements
    magnetometerDataCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    magnetometerBearingCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    magnetometerPeriodCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);

    GattCharacteristic *characteristics[] = {&magnetometerDataCharacteristic, &magnetometerBearingCharacteristic, &magnetometerPeriodCharacteristic};
    GattService         service(MicroBitMagnetometerServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    magnetometerDataCharacteristicHandle = magnetometerDataCharacteristic.getValueHandle();
    magnetometerBearingCharacteristicHandle = magnetometerBearingCharacteristic.getValueHandle();
    magnetometerPeriodCharacteristicHandle = magnetometerPeriodCharacteristic.getValueHandle();

    ble.gattServer().notify(magnetometerDataCharacteristicHandle,(uint8_t *)magnetometerDataCharacteristicBuffer, sizeof(magnetometerDataCharacteristicBuffer));
    ble.gattServer().notify(magnetometerBearingCharacteristicHandle,(uint8_t *)&magnetometerBearingCharacteristicBuffer, sizeof(magnetometerDataCharacteristicBuffer));
    ble.gattServer().write(magnetometerPeriodCharacteristicHandle, (const uint8_t *)&magnetometerPeriodCharacteristicBuffer, sizeof(magnetometerPeriodCharacteristicBuffer));

    ble.onDataWritten(this, &MicroBitMagnetometerService::onDataWritten);
    uBit.MessageBus.listen(MICROBIT_ID_COMPASS, MICROBIT_COMPASS_EVT_DATA_UPDATE, this, &MicroBitMagnetometerService::magnetometerUpdate, MESSAGE_BUS_LISTENER_IMMEDIATE);
    uBit.MessageBus.listen(MICROBIT_ID_COMPASS, MICROBIT_COMPASS_EVT_CONFIG_NEEDED, this, &MicroBitMagnetometerService::samplePeriodUpdateNeeded);
}

/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitMagnetometerService::onDataWritten(const GattWriteCallbackParams *params)
{   
    if (params->handle == magnetometerPeriodCharacteristicHandle && params->len >= sizeof(magnetometerPeriodCharacteristicBuffer))
    {
        magnetometerPeriodCharacteristicBuffer = *((uint16_t *)params->data);
        MicroBitEvent evt(MICROBIT_ID_COMPASS, MICROBIT_COMPASS_EVT_CONFIG_NEEDED);
    }
}

/**
  * Magnetometer update callback
  */
void MicroBitMagnetometerService::magnetometerUpdate(MicroBitEvent)
{
    if (ble.getGapState().connected)
    {
        magnetometerDataCharacteristicBuffer[0] = uBit.compass.getX();
        magnetometerDataCharacteristicBuffer[1] = uBit.compass.getY();
        magnetometerDataCharacteristicBuffer[2] = uBit.compass.getZ();
        magnetometerPeriodCharacteristicBuffer = uBit.compass.getPeriod();

        ble.gattServer().write(magnetometerPeriodCharacteristicHandle, (const uint8_t *)&magnetometerPeriodCharacteristicBuffer, sizeof(magnetometerPeriodCharacteristicBuffer));
        ble.gattServer().notify(magnetometerDataCharacteristicHandle,(uint8_t *)magnetometerDataCharacteristicBuffer, sizeof(magnetometerDataCharacteristicBuffer));

        if (uBit.compass.isCalibrated())
        {
            magnetometerBearingCharacteristicBuffer = (uint16_t) uBit.compass.heading();
            ble.gattServer().notify(magnetometerBearingCharacteristicHandle,(uint8_t *)&magnetometerBearingCharacteristicBuffer, sizeof(magnetometerBearingCharacteristicBuffer));
        }
    }
}

/**
 * Sample Period Change Needed callback.
 * Reconfiguring the magnetometer can to a REALLY long time (sometimes even seconds to complete)
 * So we do this in the background when necessary, through this event handler. 
 */
void MicroBitMagnetometerService::samplePeriodUpdateNeeded(MicroBitEvent)
{
    // Reconfigure the compass. This might take a while...
    uBit.compass.setPeriod(magnetometerPeriodCharacteristicBuffer);

    // The compass will choose the nearest sample period to that we've specified.
    // Read the ACTUAL sample period back.
    magnetometerPeriodCharacteristicBuffer = uBit.compass.getPeriod();

    // Ensure this is reflected in our BLE connection.
    ble.gattServer().write(magnetometerPeriodCharacteristicHandle, (const uint8_t *)&magnetometerPeriodCharacteristicBuffer, sizeof(magnetometerPeriodCharacteristicBuffer));

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

const uint8_t  MicroBitMagnetometerServiceBearingUUID[] = {
    0xe9,0x5d,0x97,0x15,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};


