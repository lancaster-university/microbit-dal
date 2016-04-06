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
#include "ble/BLE.h"
// MicroBit ControlPoint OpCodes

// Requests transfer to the Nordic DFU bootloader.
// This will only occur if 
#define MICROBIT_DFU_OPCODE_START_DFU       1   
#define MICROBIT_DFU_OPCODE_START_PAIR      2

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
  *
  * This is actually just a frontend to a memory resident nordic DFU loader.
  * Here we deal with the MicroBit 'pairing' functionality with BLE devices, and
  * very basic authentication and authorization. 
  *
  * This implementation is not intended to be fully secure, but rather intends to:
  *
  * 1. Provide a simple mechanism to identify an individual MicroBit amongst a classroom of others
  * 2. Allow BLE devices to discover and cache a passcode that can be used to flash the device over BLE.
  * 3. Provide an escape route for programs that 'brick' the MicroBit. 
  *
  * Represents the device as a whole, and includes member variables to that reflect the components of the system.
  */
class MicroBitDFUService
{                                    
    public:
    
    /**
      * Constructor. 
      * Create a representation of a MicroBit device.
      * @param messageBus callback function to receive MicroBitMessageBus events.
      */
    MicroBitDFUService(BLEDevice &BLE);  
    
    /**
      * Begin the pairing process. Typically called when device is powered up with buttons held down.
      * Scroll a description on the display, then displays the device ID code as a histogram on the matrix display.
      */
    void pair();
    
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
