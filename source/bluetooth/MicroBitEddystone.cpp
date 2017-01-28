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

#include "MicroBitConfig.h"
#include "MicroBitEddystone.h"

MicroBitEddystone *MicroBitEddystone::_instance = NULL;

/* The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ.
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
#if !defined(__arm)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/*
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif

const uint8_t EDDYSTONE_UUID[] = {0xAA, 0xFE};

#if CONFIG_ENABLED(MICROBIT_BLE_EDDYSTONE_URL)
const char *EDDYSTONE_URL_PREFIXES[] = {"http://www.", "https://www.", "http://", "https://"};
const size_t EDDYSTONE_URL_PREFIXES_LENGTH = sizeof(EDDYSTONE_URL_PREFIXES) / sizeof(char *);
const char *EDDYSTONE_URL_SUFFIXES[] = {".com/", ".org/", ".edu/", ".net/", ".info/", ".biz/", ".gov/", ".com", ".org", ".edu", ".net", ".info", ".biz", ".gov"};
const size_t EDDYSTONE_URL_SUFFIXES_LENGTH = sizeof(EDDYSTONE_URL_SUFFIXES) / sizeof(char *);
const int EDDYSTONE_URL_MAX_LENGTH = 18;
const uint8_t EDDYSTONE_URL_FRAME_TYPE = 0x10;
#endif

#if CONFIG_ENABLED(MICROBIT_BLE_EDDYSTONE_UID)
const int EDDYSTONE_UID_NAMESPACE_MAX_LENGTH = 10;
const int EDDYSTONE_UID_INSTANCE_MAX_LENGTH = 6;
const uint8_t EDDYSTONE_UID_FRAME_TYPE = 0x00;
#endif

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
MicroBitEddystone::MicroBitEddystone()
{
}

MicroBitEddystone* MicroBitEddystone::getInstance()
{
    if (_instance == 0)
        _instance = new MicroBitEddystone;

    return _instance;
}

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
int MicroBitEddystone::setURL(BLEDevice* ble, const char* url, int8_t calibratedPower)
{
    int urlDataLength = 0;
    char urlData[EDDYSTONE_URL_MAX_LENGTH];
    memset(urlData, 0, EDDYSTONE_URL_MAX_LENGTH);

    if (url == NULL || strlen(url) == 0)
        return MICROBIT_INVALID_PARAMETER;

    // Prefix
    for (size_t i = 0; i < EDDYSTONE_URL_PREFIXES_LENGTH; i++)
    {
        size_t prefixLen = strlen(EDDYSTONE_URL_PREFIXES[i]);
        if (strncmp(url, EDDYSTONE_URL_PREFIXES[i], prefixLen) == 0)
        {
            urlData[urlDataLength++] = i;
            url += prefixLen;
            break;
        }
    }

    // Suffix
    while (*url && (urlDataLength < EDDYSTONE_URL_MAX_LENGTH))
    {
        size_t i;
        for (i = 0; i < EDDYSTONE_URL_SUFFIXES_LENGTH; i++)
        {
            size_t suffixLen = strlen(EDDYSTONE_URL_SUFFIXES[i]);
            if (strncmp(url, EDDYSTONE_URL_SUFFIXES[i], suffixLen) == 0)
            {
                urlData[urlDataLength++] = i;
                url += suffixLen;
                break;
            }
        }

        // Catch the default case where the suffix doesn't match a preset ones
        if (i == EDDYSTONE_URL_SUFFIXES_LENGTH)
        {
            urlData[urlDataLength++] = *url;
            ++url;
        }
    }

    uint8_t rawFrame[EDDYSTONE_URL_MAX_LENGTH + 4];
    size_t index = 0;
    rawFrame[index++] = EDDYSTONE_UUID[0];
    rawFrame[index++] = EDDYSTONE_UUID[1];
    rawFrame[index++] = EDDYSTONE_URL_FRAME_TYPE;
    rawFrame[index++] = calibratedPower;
    memcpy(rawFrame + index, urlData, urlDataLength);

    ble->accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, EDDYSTONE_UUID, sizeof(EDDYSTONE_UUID));
    ble->accumulateAdvertisingPayload(GapAdvertisingData::SERVICE_DATA, rawFrame, index + urlDataLength);

    return MICROBIT_OK;
}

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
int MicroBitEddystone::setURL(BLEDevice* ble, ManagedString url, int8_t calibratedPower)
{
    return setURL(ble, (char *)url.toCharArray(), calibratedPower);
}
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
int MicroBitEddystone::setUID(BLEDevice* ble, const char* uid_namespace, const char* uid_instance, int8_t calibratedPower)
{
    if (uid_namespace == NULL || uid_instance == NULL)
        return MICROBIT_INVALID_PARAMETER;

    char uidData[EDDYSTONE_UID_NAMESPACE_MAX_LENGTH + EDDYSTONE_UID_INSTANCE_MAX_LENGTH];

    // UID namespace
    memcpy(uidData, uid_namespace, EDDYSTONE_UID_NAMESPACE_MAX_LENGTH);

    // UID instance
    memcpy(uidData + EDDYSTONE_UID_NAMESPACE_MAX_LENGTH, uid_instance, EDDYSTONE_UID_INSTANCE_MAX_LENGTH);

    uint8_t rawFrame[EDDYSTONE_UID_NAMESPACE_MAX_LENGTH + EDDYSTONE_UID_INSTANCE_MAX_LENGTH + 4];
    size_t index = 0;
    rawFrame[index++] = EDDYSTONE_UUID[0];
    rawFrame[index++] = EDDYSTONE_UUID[1];
    rawFrame[index++] = EDDYSTONE_UID_FRAME_TYPE;
    rawFrame[index++] = calibratedPower;
    memcpy(rawFrame + index, uidData, EDDYSTONE_UID_NAMESPACE_MAX_LENGTH + EDDYSTONE_UID_INSTANCE_MAX_LENGTH);

    ble->accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, EDDYSTONE_UUID, sizeof(EDDYSTONE_UUID));
    ble->accumulateAdvertisingPayload(GapAdvertisingData::SERVICE_DATA, rawFrame, index + EDDYSTONE_UID_NAMESPACE_MAX_LENGTH + EDDYSTONE_UID_INSTANCE_MAX_LENGTH);

    return MICROBIT_OK;
}

#endif
