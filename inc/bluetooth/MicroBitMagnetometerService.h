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

#ifndef MICROBIT_MAGNETOMETER_SERVICE_H
#define MICROBIT_MAGNETOMETER_SERVICE_H

#include "ble/BLE.h"
#include "MicroBitConfig.h"
#include "MicroBitCompass.h"
#include "EventModel.h"

#define COMPASS_CALIBRATION_STATUS_UNKNOWN 0
#define COMPASS_CALIBRATION_REQUESTED      1
#define COMPASS_CALIBRATION_COMPLETED_OK   2
#define COMPASS_CALIBRATION_COMPLETED_ERR  3

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitMagnetometerServiceUUID[];
extern const uint8_t  MicroBitMagnetometerServiceDataUUID[];
extern const uint8_t  MicroBitMagnetometerServiceBearingUUID[];
extern const uint8_t  MicroBitMagnetometerServicePeriodUUID[];
extern const uint8_t  MicroBitMagnetometerServiceCalibrationUUID[];

/**
  * Class definition for the MicroBit BLE Magnetometer Service.
  * Provides access to live magnetometer data via BLE, and provides basic configuration options.
  */
class MicroBitMagnetometerService
{
    public:

    /**
      * Constructor.
      * Create a representation of the MagnetometerService.
      * @param _ble The instance of a BLE device that we're running on.
      * @param _compass An instance of MicroBitCompass to use as our Magnetometer source.
      */
    MicroBitMagnetometerService(BLEDevice &_ble, MicroBitCompass &_compass);

    private:

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);

    /**
     * Magnetometer update callback
     */
    void magnetometerUpdate();

    /**
     * Sample Period Change Needed callback.
     * Reconfiguring the magnetometer can to a REALLY long time (sometimes even seconds to complete)
     * So we do this in the background when necessary, through this event handler.
     */
    void samplePeriodUpdateNeeded();

    /**
     *calibrate compass
     */
    void calibrateCompass();

    /**
     * Handle compass events such as calibration requests
     */
    void compassEvents(MicroBitEvent e);

    // Bluetooth stack we're running on.
    BLEDevice           &ble;
    MicroBitCompass     &compass;

    // memory for our 8 bit control characteristics.
    int16_t             magnetometerDataCharacteristicBuffer[3];
    uint16_t            magnetometerBearingCharacteristicBuffer;
    uint16_t            magnetometerPeriodCharacteristicBuffer;
    uint8_t             magnetometerCalibrationCharacteristicBuffer;

    // Handles to access each characteristic when they are held by Soft Device.
    GattAttribute::Handle_t magnetometerDataCharacteristicHandle;
    GattAttribute::Handle_t magnetometerBearingCharacteristicHandle;
    GattAttribute::Handle_t magnetometerPeriodCharacteristicHandle;
    GattAttribute::Handle_t magnetometerCalibrationCharacteristicHandle;
};

#endif
