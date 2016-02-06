/**
  * Class definition for the custom MicroBit Accelerometer Service.
  * Provides a BLE service to remotely read the state of the accelerometer, and configure its behaviour.
  */
  
#include "MicroBit.h"
#include "ble/UUID.h"

#include "MicroBitAccelerometerService.h"

/**
  * Constructor. 
  * Create a representation of the AccelerometerService
  * @param _ble The instance of a BLE device that we're running on.
  */
MicroBitAccelerometerService::MicroBitAccelerometerService(BLEDevice &_ble) : 
        ble(_ble) 
{
    // Create the data structures that represent each of our characteristics in Soft Device.
    GattCharacteristic  accelerometerDataCharacteristic(MicroBitAccelerometerServiceDataUUID, (uint8_t *)accelerometerDataCharacteristicBuffer, 0, 
    sizeof(accelerometerDataCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    GattCharacteristic  accelerometerPeriodCharacteristic(MicroBitAccelerometerServicePeriodUUID, (uint8_t *)&accelerometerPeriodCharacteristicBuffer, 0, 
    sizeof(accelerometerPeriodCharacteristicBuffer), 
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);

    // Initialise our characteristic values.
    accelerometerDataCharacteristicBuffer[0] = 0;
    accelerometerDataCharacteristicBuffer[1] = 0;
    accelerometerDataCharacteristicBuffer[2] = 0;
    accelerometerPeriodCharacteristicBuffer = uBit.accelerometer.getPeriod();

    // Set default security requirements
    accelerometerDataCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    accelerometerPeriodCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);

    GattCharacteristic *characteristics[] = {&accelerometerDataCharacteristic, &accelerometerPeriodCharacteristic};
    GattService         service(MicroBitAccelerometerServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    accelerometerDataCharacteristicHandle = accelerometerDataCharacteristic.getValueHandle();
    accelerometerPeriodCharacteristicHandle = accelerometerPeriodCharacteristic.getValueHandle();

    ble.gattServer().write(accelerometerDataCharacteristicHandle,(uint8_t *)accelerometerDataCharacteristicBuffer, sizeof(accelerometerDataCharacteristicBuffer));
    ble.gattServer().write(accelerometerPeriodCharacteristicHandle, (const uint8_t *)&accelerometerPeriodCharacteristicBuffer, sizeof(accelerometerPeriodCharacteristicBuffer));

    ble.onDataWritten(this, &MicroBitAccelerometerService::onDataWritten);
    uBit.MessageBus.listen(MICROBIT_ID_ACCELEROMETER, MICROBIT_ACCELEROMETER_EVT_DATA_UPDATE, this, &MicroBitAccelerometerService::accelerometerUpdate,  MESSAGE_BUS_LISTENER_IMMEDIATE);
}

/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitAccelerometerService::onDataWritten(const GattWriteCallbackParams *params)
{   
    if (params->handle == accelerometerPeriodCharacteristicHandle && params->len >= sizeof(accelerometerPeriodCharacteristicBuffer))
    {
        accelerometerPeriodCharacteristicBuffer = *((uint16_t *)params->data);
        uBit.accelerometer.setPeriod(accelerometerPeriodCharacteristicBuffer);

        // The accelerometer will choose the nearest period to that requested that it can support
        // Read back the ACTUAL period it is using, and report this back.
        accelerometerPeriodCharacteristicBuffer = uBit.accelerometer.getPeriod();
        ble.gattServer().write(accelerometerPeriodCharacteristicHandle, (const uint8_t *)&accelerometerPeriodCharacteristicBuffer, sizeof(accelerometerPeriodCharacteristicBuffer));
    }
}

/**
  * Accelerometer update callback
  */
void MicroBitAccelerometerService::accelerometerUpdate(MicroBitEvent)
{
    if (ble.getGapState().connected)
    {
		accelerometerDataCharacteristicBuffer[0] = uBit.accelerometer.getX();
		accelerometerDataCharacteristicBuffer[1] = uBit.accelerometer.getY();
		accelerometerDataCharacteristicBuffer[2] = uBit.accelerometer.getZ();

		ble.gattServer().notify(accelerometerDataCharacteristicHandle,(uint8_t *)accelerometerDataCharacteristicBuffer, sizeof(accelerometerDataCharacteristicBuffer));
	}
}

const uint8_t  MicroBitAccelerometerServiceUUID[] = {
    0xe9,0x5d,0x07,0x53,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitAccelerometerServiceDataUUID[] = {
    0xe9,0x5d,0xca,0x4b,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitAccelerometerServicePeriodUUID[] = {
    0xe9,0x5d,0xfb,0x24,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};


