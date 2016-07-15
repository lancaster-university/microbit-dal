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

#ifndef MICROBIT_DEVICE_INFORMATION_SERVICE_H
#define MICROBIT_DEVICE_INFORMATION_SERVICE_H

#include "MicroBitConfig.h"
#include "ManagedString.h"
#include "ble/BLE.h"
#include "ble/services/DeviceInformationService.h"

/**
  * Class definition for the microbit's device informiton service.
  * Simply uses mbed's DeviceInformation service with the default parameters for a micro:bit.
  */
class MicroBitDeviceInformationService
{
    public:
    static MicroBitDeviceInformationService *instance;

    /**
      * Constructor.
      * Create a representation of the DeviceInformationService
      * @param _ble The instance of a BLE device that we're running on.
      */
    MicroBitDeviceInformationService(BLEDevice &_ble);

    /**
     * Singleton constructor.
     * Create a representation of the DeviceInformationService, unless one has already been created.
     * If one has been created, this is returned to the caller.
     * 
     * @param _ble The instance of a BLE device that we're running on.
     * @return a MicroBitDeviceInformationService.
     */
    MicroBitDeviceInformationService* getInstance(BLEDevice &_ble);
};


#endif
