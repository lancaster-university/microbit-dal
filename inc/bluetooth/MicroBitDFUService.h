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

#ifndef MICROBIT_DFU_SERVICE_H
#define MICROBIT_DFU_SERVICE_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "ble/BLE.h"
#include "MicroBitEvent.h"

// MicroBit ControlPoint OpCodes
// Requests transfer to the Nordic DFU bootloader.
#define MICROBIT_DFU_OPCODE_START_DFU       1

// visual ID code constants
#define MICROBIT_DFU_HISTOGRAM_WIDTH        5
#define MICROBIT_DFU_HISTOGRAM_HEIGHT       5

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitDFUServiceUUID[];
extern const uint8_t  MicroBitDFUServiceControlCharacteristicUUID[];
extern const uint8_t  MicroBitDFUServiceFlashCodeCharacteristicUUID[];

// Handle on the memory resident Nordic bootloader.
extern "C" void bootloader_start(void);

/**
  * Class definition for a MicroBit Device Firmware Update loader.
  * This service allows hexes to be flashed remotely from another Bluetooth
  * device.
  */
class MicroBitDFUService
{
    public:

    /**
      * Constructor.
      * Initialise the Device Firmware Update service.
      * @param _ble The instance of a BLE device that we're running on.
      */
    MicroBitDFUService(BLEDevice &_ble);

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    virtual void onDataWritten(const GattWriteCallbackParams *params);

    private:

    // State of paiting process.
    bool authenticated;
    bool flashCodeRequested;

    // Bluetooth stack we're running on.
    BLEDevice           &ble;

    // memory for our 8 bit control characteristic.
    uint8_t             controlByte;

    // BLE pairing name of this device, encoded as an integer.
    uint32_t            flashCode;

    GattAttribute::Handle_t microBitDFUServiceControlCharacteristicHandle;
    GattAttribute::Handle_t microBitDFUServiceFlashCodeCharacteristicHandle;

    // Displays the device's ID code as a histogram on the LED matrix display.
    void showNameHistogram();

    // Displays an acknowledgement on the LED matrix display.
    void showTick();

    // Update BLE characteristic to release our flash code.
    void releaseFlashCode();

    // Event handlers for button clicks.
    void onButtonA(MicroBitEvent e);
    void onButtonB(MicroBitEvent e);
};

#endif
