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
  * Class definition for the custom MicroBit Temperature Service.
  * Provides a BLE service to remotely read the silicon temperature of the nRF51822.
  */
#include "MicroBitConfig.h"
#include "ble/UUID.h"

#include "MicroBitDeviceInformationService.h"

MicroBitDeviceInformationService* MicroBitDeviceInformationService::instance = NULL;

static const char* MICROBIT_BLE_MANUFACTURER = NULL;
static const char* MICROBIT_BLE_MODEL = "BBC micro:bit";
static const char* MICROBIT_BLE_HARDWARE_VERSION = NULL;
static const char* MICROBIT_BLE_FIRMWARE_VERSION = MICROBIT_DAL_VERSION;
static const char* MICROBIT_BLE_SOFTWARE_VERSION = NULL;

/**
  * Constructor.
  * Create a representation of the TemperatureService
  * @param _ble The instance of a BLE device that we're running on.
  * @param _thermometer An instance of MicroBitThermometer to use as our temperature source.
  */
MicroBitDeviceInformationService::MicroBitDeviceInformationService(MicroBitBLEManager &_ble)
{
    int n1 = microbit_serial_number() & 0xffff;
    int n2 = (microbit_serial_number() >> 16) & 0xffff;

    // If the memory of associated with the BLE stack has been recycled, it isn't safe to add more services.
    if(_ble.isLocked())
        return;

    DeviceInformationService ble_device_information_service (_ble.ble, MICROBIT_BLE_MANUFACTURER, MICROBIT_BLE_MODEL, (ManagedString(n1) + ManagedString(n2)).toCharArray(), MICROBIT_BLE_HARDWARE_VERSION, MICROBIT_BLE_FIRMWARE_VERSION, MICROBIT_BLE_SOFTWARE_VERSION);
}

/**
 * Singleton constructor.
 * Create a representation of the DeviceInformationService, unless one has already been created.
 * If one has been created, this is returned to the caller.
 * 
 * @param _ble The instance of a BLE device that we're running on.
 * @return a MicroBitDeviceInformationService.
 */
MicroBitDeviceInformationService* MicroBitDeviceInformationService::getInstance(MicroBitBLEManager &_ble)
{
    if (instance == NULL)
       instance = new MicroBitDeviceInformationService(_ble); 

    return instance;
}

