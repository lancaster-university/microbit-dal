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

#ifndef MICROBIT_ACCELEROMETER_SERVICE_H
#define MICROBIT_ACCELEROMETER_SERVICE_H

#include "MicroBitConfig.h"
#include "ble/BLE.h"
#include "MicroBitAccelerometer.h"
#include "EventModel.h"

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitAccelerometerServiceUUID[];
extern const uint8_t  MicroBitAccelerometerServiceDataUUID[];
extern const uint8_t  MicroBitAccelerometerServicePeriodUUID[];


/**
  * Class definition for a MicroBit BLE Accelerometer Service.
  * Provides access to live accelerometer data via Bluetooth, and provides basic configuration options.
  */
class MicroBitAccelerometerService
{
    public:

    /**
      * Constructor.
      * Create a representation of the AccelerometerService
      * @param _ble The instance of a BLE device that we're running on.
      * @param _accelerometer An instance of MicroBitAccelerometer.
      */
    MicroBitAccelerometerService(BLEDevice &_ble, MicroBitAccelerometer &_acclerometer);


    private:

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);

    /**
     * Accelerometer update callback
     */
    void accelerometerUpdate(MicroBitEvent e);

    // Bluetooth stack we're running on.
    BLEDevice           	&ble;
	MicroBitAccelerometer	&accelerometer;

    // memory for our 8 bit control characteristics.
    uint16_t            accelerometerDataCharacteristicBuffer[3];
    uint16_t            accelerometerPeriodCharacteristicBuffer;

    // Handles to access each characteristic when they are held by Soft Device.
    GattAttribute::Handle_t accelerometerDataCharacteristicHandle;
    GattAttribute::Handle_t accelerometerPeriodCharacteristicHandle;
};


#endif
