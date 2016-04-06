/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

/**
  * Class definition for the custom MicroBit Accelerometer Service.
  * Provides a BLE service to remotely read the state of the accelerometer, and configure its behaviour.
  */
#include "MicroBitConfig.h"
#include "ble/UUID.h"

#include "MicroBitAccelerometerService.h"

/**
  * Constructor.
  * Create a representation of the AccelerometerService
  * @param _ble The instance of a BLE device that we're running on.
  * @param _accelerometer An instance of MicroBitAccelerometer.
  */
MicroBitAccelerometerService::MicroBitAccelerometerService(BLEDevice &_ble, MicroBitAccelerometer &_accelerometer) :
        ble(_ble), accelerometer(_accelerometer)
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
    accelerometerPeriodCharacteristicBuffer = accelerometer.getPeriod();

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

    if (EventModel::defaultEventBus)
        EventModel::defaultEventBus->listen(MICROBIT_ID_ACCELEROMETER, MICROBIT_ACCELEROMETER_EVT_DATA_UPDATE, this, &MicroBitAccelerometerService::accelerometerUpdate,  MESSAGE_BUS_LISTENER_IMMEDIATE);
}

/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitAccelerometerService::onDataWritten(const GattWriteCallbackParams *params)
{
    if (params->handle == accelerometerPeriodCharacteristicHandle && params->len >= sizeof(accelerometerPeriodCharacteristicBuffer))
    {
        accelerometerPeriodCharacteristicBuffer = *((uint16_t *)params->data);
        accelerometer.setPeriod(accelerometerPeriodCharacteristicBuffer);

        // The accelerometer will choose the nearest period to that requested that it can support
        // Read back the ACTUAL period it is using, and report this back.
        accelerometerPeriodCharacteristicBuffer = accelerometer.getPeriod();
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
        accelerometerDataCharacteristicBuffer[0] = accelerometer.getX();
        accelerometerDataCharacteristicBuffer[1] = accelerometer.getY();
        accelerometerDataCharacteristicBuffer[2] = accelerometer.getZ();

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
