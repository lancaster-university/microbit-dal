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

#ifndef MICROBIT_EDDYSTONE_H
#define MICROBIT_EDDYSTONE_H

#include "MicroBitConfig.h"

/*
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif

#include "MicroBitBLEManager.h"

/**
  * Class definition for the MicroBitEddystone.
  *
  */
class MicroBitEddystone
{
  public:

    static MicroBitEddystone* getInstance();

#if CONFIG_ENABLED(MICROBIT_BLE_EDDYSTONE_URL)

    /**
      * Set the content of Eddystone URL frames
      *
      * @param url The url to broadcast
      *
      * @param calibratedPower the transmission range of the beacon (Defaults to: 0xF0 ~10m).
      *
      * @note The calibratedPower value ranges from -100 to +20 to a resolution of 1. The calibrated power should be binary encoded.
      * More information can be found at https://github.com/google/eddystone/tree/master/eddystone-uid#tx-power
      */
    int setURL(BLEDevice* ble, const char *url, int8_t calibratedPower = 0xF0);

    /**
      * Set the content of Eddystone URL frames, but accepts a ManagedString as a url.
      *
      * @param url The url to broadcast
      *
      * @param calibratedPower the transmission range of the beacon (Defaults to: 0xF0 ~10m).
      *
      * @note The calibratedPower value ranges from -100 to +20 to a resolution of 1. The calibrated power should be binary encoded.
      * More information can be found at https://github.com/google/eddystone/tree/master/eddystone-uid#tx-power
      */
    int setURL(BLEDevice* ble, ManagedString url, int8_t calibratedPower = 0xF0);

#endif

#if CONFIG_ENABLED(MICROBIT_BLE_EDDYSTONE_UID)
    /**
      * Set the content of Eddystone UID frames
      *
      * @param uid_namespace the uid namespace. Must 10 bytes long.
      *
      * @param uid_instance  the uid instance value. Must 6 bytes long.
      *
      * @param calibratedPower the transmission range of the beacon (Defaults to: 0xF0 ~10m).
      *
      * @note The calibratedPower value ranges from -100 to +20 to a resolution of 1. The calibrated power should be binary encoded.
      * More information can be found at https://github.com/google/eddystone/tree/master/eddystone-uid#tx-power
      */
    int setUID(BLEDevice* ble, const char* uid_namespace, const char* uid_instance, int8_t calibratedPower = 0xF0);
#endif

  private:

    /**
      * Private constructor.
      */
    MicroBitEddystone();

    static MicroBitEddystone *_instance;
};

#endif
