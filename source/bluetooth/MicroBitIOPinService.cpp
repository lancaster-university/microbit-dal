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
  * Class definition for the custom MicroBit IOPin Service.
  * Provides a BLE service to remotely read the state of the I/O Pin, and configure its behaviour.
  */
#include "MicroBitConfig.h"
#include "ble/UUID.h"

#include "MicroBitIOPinService.h"
#include "MicroBitFiber.h"

/**
  * Constructor.
  * Create a representation of the IOPinService
  * @param _ble The instance of a BLE device that we're running on.
  * @param _io An instance of MicroBitIO that this service will use to perform
  *            I/O operations.
  */
MicroBitIOPinService::MicroBitIOPinService(BLEDevice &_ble, MicroBitIO &_io) :
        ble(_ble), io(_io)
{
    // Create the AD characteristic, that defines whether each pin is treated as analogue or digital
    GattCharacteristic ioPinServiceADCharacteristic(MicroBitIOPinServiceADConfigurationUUID, (uint8_t *)&ioPinServiceADCharacteristicBuffer, 0, sizeof(ioPinServiceADCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);

    // Create the IO characteristic, that defines whether each pin is treated as input or output
    GattCharacteristic ioPinServiceIOCharacteristic(MicroBitIOPinServiceIOConfigurationUUID, (uint8_t *)&ioPinServiceIOCharacteristicBuffer, 0, sizeof(ioPinServiceIOCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);

    // Create the PWM characteristic, that allows up to 3 compatible pins to be used for PWM
    GattCharacteristic ioPinServicePWMCharacteristic(MicroBitIOPinServicePWMControlUUID, (uint8_t *)&ioPinServicePWMCharacteristicBuffer, 0, sizeof(ioPinServicePWMCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);

    // Create the Data characteristic, that allows the actual read and write operations.
    ioPinServiceDataCharacteristic = new GattCharacteristic(MicroBitIOPinServiceDataUUID, (uint8_t *)ioPinServiceDataCharacteristicBuffer, 0, sizeof(ioPinServiceDataCharacteristicBuffer), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    ioPinServiceDataCharacteristic->setReadAuthorizationCallback(this, &MicroBitIOPinService::onDataRead);

    ioPinServiceADCharacteristicBuffer = 0;
    ioPinServiceIOCharacteristicBuffer = 0;
    memset(ioPinServiceIOData, 0, sizeof(ioPinServiceIOData));
    memset(ioPinServicePWMCharacteristicBuffer, 0, sizeof(ioPinServicePWMCharacteristicBuffer));

    // Set default security requirements
    ioPinServiceADCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    ioPinServiceIOCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    ioPinServicePWMCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    ioPinServiceDataCharacteristic->requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);

    GattCharacteristic *characteristics[] = {&ioPinServiceADCharacteristic, &ioPinServiceIOCharacteristic, &ioPinServicePWMCharacteristic, ioPinServiceDataCharacteristic};
    GattService         service(MicroBitIOPinServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    ioPinServiceADCharacteristicHandle = ioPinServiceADCharacteristic.getValueHandle();
    ioPinServiceIOCharacteristicHandle = ioPinServiceIOCharacteristic.getValueHandle();
    ioPinServicePWMCharacteristicHandle = ioPinServicePWMCharacteristic.getValueHandle();

    ble.gattServer().write(ioPinServiceADCharacteristicHandle, (const uint8_t *)&ioPinServiceADCharacteristicBuffer, sizeof(ioPinServiceADCharacteristicBuffer));
    ble.gattServer().write(ioPinServiceIOCharacteristicHandle, (const uint8_t *)&ioPinServiceIOCharacteristicBuffer, sizeof(ioPinServiceIOCharacteristicBuffer));
    ble.gattServer().write(ioPinServicePWMCharacteristicHandle, (const uint8_t *)&ioPinServicePWMCharacteristicBuffer, sizeof(ioPinServicePWMCharacteristicBuffer));

    ble.onDataWritten(this, &MicroBitIOPinService::onDataWritten);
    fiber_add_idle_component(this);
}

/**
  * Determines if the given pin was configured as a digital pin by the BLE ADPinConfigurationCharacterisitic.
  *
  * @param i the enumeration of the pin to test
  * @return 1 if this pin is configured as digital, 0 otherwise
  */
int MicroBitIOPinService::isDigital(int i)
{
    return ((ioPinServiceADCharacteristicBuffer & (1 << i)) == 0);
}

/**
  * Determines if the given pin was configured as an analog pin by the BLE ADPinConfigurationCharacterisitic.
  *
  * @param i the enumeration of the pin to test
  * @return 1 if this pin is configured as analog, 0 otherwise
  */
int MicroBitIOPinService::isAnalog(int i)
{
    return ((ioPinServiceADCharacteristicBuffer & (1 << i)) != 0);
}

/**
  * Determines if the given pin was configured as an input by the BLE IOPinConfigurationCharacterisitic.
  *
  * @param i the enumeration of the pin to test
  * @return 1 if this pin is configured as an input, 0 otherwise
  */
int MicroBitIOPinService::isInput(int i)
{
    return ((ioPinServiceIOCharacteristicBuffer & (1 << i)) != 0);
}

/**
  * Determines if the given pin was configured as output by the BLE IOPinConfigurationCharacterisitic.
  *
  * @param i the enumeration of the pin to test
  * @return 1 if this pin is configured as an output, 0 otherwise
  */
int MicroBitIOPinService::isOutput(int i)
{
    return ((ioPinServiceIOCharacteristicBuffer & (1 << i)) == 0);
}

/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitIOPinService::onDataWritten(const GattWriteCallbackParams *params)
{
    // Check for writes to the IO configuration characteristic
    if (params->handle == ioPinServiceIOCharacteristicHandle && params->len >= sizeof(ioPinServiceIOCharacteristicBuffer))
    {
        uint32_t *value = (uint32_t *)params->data;

        // Our IO configuration may be changing... read the new value, and push it back into the BLE stack.
        ioPinServiceIOCharacteristicBuffer = *value;
        ble.gattServer().write(ioPinServiceIOCharacteristicHandle, (const uint8_t *)&ioPinServiceIOCharacteristicBuffer, sizeof(ioPinServiceIOCharacteristicBuffer));

        // Also, drop any selected pins into input mode, so we can pick up changes later
        for (int i=0; i < MICROBIT_IO_PIN_SERVICE_PINCOUNT; i++)
        {
            if(isDigital(i) && isInput(i))
               io.pin[i].getDigitalValue();
               //MicroBitIOPins[i]->getDigitalValue();

            if(isAnalog(i) && isInput(i))
               io.pin[i].getAnalogValue();
               //MicroBitIOPins[i]->getAnalogValue();
        }
    }

    // Check for writes to the IO configuration characteristic
    if (params->handle == ioPinServiceADCharacteristicHandle && params->len >= sizeof(ioPinServiceADCharacteristicBuffer))
    {
        uint32_t *value = (uint32_t *)params->data;

        // Our IO configuration may be changing... read the new value, and push it back into the BLE stack.
        ioPinServiceADCharacteristicBuffer = *value;
        ble.gattServer().write(ioPinServiceADCharacteristicHandle, (const uint8_t *)&ioPinServiceADCharacteristicBuffer, sizeof(ioPinServiceADCharacteristicBuffer));

        // Also, drop any selected pins into input mode, so we can pick up changes later
        for (int i=0; i < MICROBIT_IO_PIN_SERVICE_PINCOUNT; i++)
        {
            if(isDigital(i) && isInput(i))
               io.pin[i].getDigitalValue();
               //MicroBitIOPins[i]->getDigitalValue();

            if(isAnalog(i) && isInput(i))
               io.pin[i].getAnalogValue();
               //MicroBitIOPins[i]->getAnalogValue();
        }
    }

    // Check for writes to the PWM Control characteristic
    if (params->handle == ioPinServicePWMCharacteristicHandle)
    {
        uint16_t len = params->len;
        IOPWMData *pwm_data = (IOPWMData *)params->data;
        
        //validate - len must be a multiple of 7 and greater than 0
        if (len == 0) {
            return;
        }

        bool is_valid_length = len % 7 == 0;
	    if (is_valid_length) {
            uint8_t field_count = len / 7;
            for (int i=0;i<field_count;i++) {
                uint8_t  pin    = pwm_data[i].pin;
                uint16_t value = pwm_data[i].value;
                uint32_t period = pwm_data[i].period;
                io.pin[pin].setAnalogValue(value);
                io.pin[pin].setAnalogPeriodUs(period);
            }
        } else {
            // there's no way to return an error response via the current mbed BLE API :-( See https://github.com/ARMmbed/ble/issues/181
            return;
        }        
    }

    if (params->handle == ioPinServiceDataCharacteristic->getValueHandle())
    {
        // We have some pin data to change...
        uint16_t len = params->len;
        IOData *data = (IOData *)params->data;

        // There may be multiple write operations... take each in turn and update the pin values
        while (len >= sizeof(IOData))
        {
            if (isOutput(data->pin))
            {
                if (isDigital(data->pin))
               		io.pin[data->pin].setDigitalValue(data->value);
                    //MicroBitIOPins[data->pin]->setDigitalValue(data->value);
                else
               		io.pin[data->pin].setAnalogValue(data->value*4);
                    //MicroBitIOPins[data->pin]->setAnalogValue(data->value*4);
            }

            data++;
            len -= sizeof(IOData);
        }
    }
}

/**
 * Callback. invoked when the BLE data characteristic is read.
 *
 * Reads all the pins marked as inputs, and updates the data stored in the characteristic.
 */
void MicroBitIOPinService::onDataRead(GattReadAuthCallbackParams *params)
{
    if (params->handle == ioPinServiceDataCharacteristic->getValueHandle())
    {

        // Scan through all pins that our BLE client may be listening for. If any have changed value, update the BLE characterisitc, and NOTIFY our client.
        int pairs = 0;

        for (int i=0; i < MICROBIT_IO_PIN_SERVICE_PINCOUNT; i++)
        {
            if (isInput(i))
            {
                uint8_t value;

                if (isDigital(i))
               		value = io.pin[i].getDigitalValue();
                    //value = MicroBitIOPins[i]->getDigitalValue();
                else
               		value = io.pin[i].getAnalogValue();
                    //value = MicroBitIOPins[i]->getAnalogValue();

                ioPinServiceIOData[i] = value;
                ioPinServiceDataCharacteristicBuffer[pairs].pin = i;
                ioPinServiceDataCharacteristicBuffer[pairs].value = value;

                pairs++;

                if (pairs >= MICROBIT_IO_PIN_SERVICE_DATA_SIZE)
                    break;
            }
        }

        // If there's any data, issue a BLE notification.
        if (pairs > 0)
            ble.gattServer().notify(ioPinServiceDataCharacteristic->getValueHandle(), (uint8_t *)ioPinServiceDataCharacteristicBuffer, pairs * sizeof(IOData));
    }
}


/**
 * Periodic callback from MicroBit scheduler.
 *
 * Check if any of the pins we're watching need updating. Notify any connected
 * device with any changes.
 */
void MicroBitIOPinService::idleTick()
{
    // If we're not we're connected, then there's nothing to do...
    if (!ble.getGapState().connected)
        return;

    // Scan through all pins that our BLE client may be listening for. If any have changed value, update the BLE characterisitc, and NOTIFY our client.
    int pairs = 0;

    for (int i=0; i < MICROBIT_IO_PIN_SERVICE_PINCOUNT; i++)
    {
        if (isInput(i))
        {
            uint8_t value;

            if (isDigital(i))
               	value = io.pin[i].getDigitalValue();
                //value = MicroBitIOPins[i]->getDigitalValue();
            else
               	value = io.pin[i].getAnalogValue();
                //value = MicroBitIOPins[i]->getAnalogValue();

            // If the data has changed, send an update.
            if (value != ioPinServiceIOData[i])
            {
                ioPinServiceIOData[i] = value;

                ioPinServiceDataCharacteristicBuffer[pairs].pin = i;
                ioPinServiceDataCharacteristicBuffer[pairs].value = value;

                pairs++;

                if (pairs >= MICROBIT_IO_PIN_SERVICE_DATA_SIZE)
                    break;
            }
        }
    }

    // If there were any changes, issue a BLE notification.
    if (pairs > 0)
        ble.gattServer().notify(ioPinServiceDataCharacteristic->getValueHandle(), (uint8_t *)ioPinServiceDataCharacteristicBuffer, pairs * sizeof(IOData));
}

const uint8_t  MicroBitIOPinServiceUUID[] = {
    0xe9,0x5d,0x12,0x7b,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitIOPinServiceIOConfigurationUUID[] = {
    0xe9,0x5d,0xb9,0xfe,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitIOPinServiceADConfigurationUUID[] = {
    0xe9,0x5d,0x58,0x99,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitIOPinServicePWMControlUUID[] = {
    0xe9,0x5d,0xd8,0x22,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t MicroBitIOPinServiceDataUUID[] = {
    0xe9,0x5d,0x8d,0x00,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

/*
MicroBitPin * const MicroBitIOPins[] = {
    &uBit.io.P0,
    &uBit.io.P1,
    &uBit.io.P2,
    &uBit.io.P3,
    &uBit.io.P4,
    &uBit.io.P5,
    &uBit.io.P6,
    &uBit.io.P7,
    &uBit.io.P8,
    &uBit.io.P9,
    &uBit.io.P10,
    &uBit.io.P11,
    &uBit.io.P12,
    &uBit.io.P13,
    &uBit.io.P14,
    &uBit.io.P15,
    &uBit.io.P16,
    &uBit.io.P19,
    &uBit.io.P20
};
*/
