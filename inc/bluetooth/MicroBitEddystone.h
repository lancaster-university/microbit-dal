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

#define MICROBIT_BLE_EDDYSTONE_URL_ADV_INTERVAL 400

/**
  * Class definition for the MicroBitEddystone.
  *
  */
class MicroBitEddystone
{
  public:
    static MicroBitEddystone *getInstance();

#if CONFIG_ENABLED(MICROBIT_BLE_EDDYSTONE_URL)

    /**
    * Set the content of Eddystone URL frames
	* @param url: the url to transmit. Must be no longer than the supported eddystone url length
	* @param calibratedPower: the calibrated to transmit at. This is the received power at 0 meters in dBm.
        * The value ranges from -100 to +20 to a resolution of 1. The calibrated power should be binary encoded.
        * More information can be found at https://github.com/google/eddystone/tree/master/eddystone-url#tx-power-level
        * @param connectable: true to keep bluetooth connectable for other services, false otherwise
        * @param interval: the advertising interval of the beacon
	*/
    void setEddystoneUrl(BLEDevice *ble, char *url, int8_t calibratedPower);

    /**
        * Set the content of Eddystone URL frames, but accepts a ManagedString as a url. For more info see
        * setEddystoneUrl(char* url, int8_t calibratedPower, bool connectable, uint16_t interval)
        */
    void setEddystoneUrl(BLEDevice *ble, ManagedString url, int8_t calibratedPower);

#endif

#if CONFIG_ENABLED(MICROBIT_BLE_EDDYSTONE_UID)
    /**
    * Set the content of Eddystone UID frames
	* @param uid_namespace: the uid namespace. Must 10 bytes long.
	* @param uid_instance:  the uid instance value. Must 6 bytes long.
	* @param calibratedPower: the calibrated to transmit at. This is the received power at 0 meters in dBm.
    * The value ranges from -100 to +20 to a resolution of 1. The calibrated power should be binary encoded.
    * More information can be found at https://github.com/google/eddystone/tree/master/eddystone-uid#tx-power
    * @param connectable: true to keep bluetooth connectable for other services, false otherwise
    * @param interval: the advertising interval of the beacon
	*/
    void setEddystoneUid(BLEDevice *ble, char *uid_namespace, char *uid_instance, int8_t calibratedPower);

    /**
        * Set the content of Eddystone URL frames, but accepts a ManagedString as a url. For more info see
        * setEddystoneUid(char* uid_namespace, char* uid_instance, int8_t calibratedPower, bool connectable, uint16_t interval)
        */
    void setEddystoneUid(BLEDevice *ble, ManagedString uid_namespace, ManagedString uid_instance, int8_t calibratedPower);

#endif

  private:
    /**
     * Constructor.
     *
     * Configure and manage the micro:bit's Bluetooth Low Energy (BLE) stack.
     *
     * @param _storage an instance of MicroBitStorage used to persist sys attribute information. (This is required for compatability with iOS).
     *
     * @note The BLE stack *cannot*  be brought up in a static context (the software simply hangs or corrupts itself).
     * Hence, the init() member function should be used to initialise the BLE stack.
     */
    MicroBitEddystone();
    static MicroBitEddystone *_instance;
};

#endif
