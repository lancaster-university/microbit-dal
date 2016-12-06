/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BLE_HID_DEVICE_INFORMATION_SERVICE_H__
#define __BLE_HID_DEVICE_INFORMATION_SERVICE_H__

#include "ble/BLE.h"

typedef struct
{
    uint8_t vendorID_source;
    uint16_t vendorID;
    uint16_t productID;
    uint16_t productVersion;
} PnPID_t;

/**
* @class HIDDeviceInformationService
* @brief BLE Device Information Service <br>
* Service: https://developer.bluetooth.org/gatt/services/Pages/ServiceViewer.aspx?u=org.bluetooth.service.device_information.xml <br>
* Manufacturer Name String Char: https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.manufacturer_name_string.xml
*/
class HIDDeviceInformationService {
public:
    /**
     * @brief Device Information Service Constructor.
     *
     * @param[ref] _ble
     *                BLE object for the underlying controller.
     * @param[in] manufacturersName
     *                This characteristic represents the name of the
     *                manufacturer of the device. The name is copied into the
     *                BLE stack during this constructor.
     * @param[in] modelNumber
     *                This characteristic represents the model number that is
     *                assigned by the device vendor. The value is copied into
     *                the BLE stack during this constructor.
     * @param[in] serialNumber
     *                This characteristic represents the serial number for a
     *                particular instance of the device.  The value is copied
     *                into the BLE stack during this constructor.
     * @param[in] hardwareRevision
     *                This characteristic represents the hardware revision for
     *                the hardware within the device. The value is copied
     *                into the BLE stack during this constructor.
     * @param[in] firmwareRevision
     *                This characteristic represents the firmware revision for
     *                the firmware within the device. The value is copied
     *                into the BLE stack during this constructor.
     * @param[in] softwareRevision
     *                This characteristic represents the software revision for
     *                the software within the device. The value is copied
     *                into the BLE stack during this constructor.
     * @param[in] pnpID
     *                This characteristic represents HID-specific information,
     *                such as vendor id, product id and version.
     */
    HIDDeviceInformationService(BLE            &_ble,
                             const char     *manufacturersName = NULL,
                             const char     *modelNumber       = NULL,
                             const char     *serialNumber      = NULL,
                             const char     *hardwareRevision  = NULL,
                             const char     *firmwareRevision  = NULL,
                             const char     *softwareRevision  = NULL,
                             PnPID_t        *PnPID             = NULL) :
        ble(_ble)
    {
        static bool serviceAdded = false; /* We should only ever need to add the information service once. */
        if (serviceAdded) {
            return;
        }

        GattCharacteristic manufacturersNameStringCharacteristic(GattCharacteristic::UUID_MANUFACTURER_NAME_STRING_CHAR,
                                              (uint8_t *)manufacturersName,
                                              (manufacturersName != NULL) ? strlen(manufacturersName) : 0, /* minLength */
                                              (manufacturersName != NULL) ? strlen(manufacturersName) : 0, /* maxLength */
                                              GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

        GattCharacteristic modelNumberStringCharacteristic(GattCharacteristic::UUID_MODEL_NUMBER_STRING_CHAR,
                                        (uint8_t *)modelNumber,
                                        (modelNumber != NULL) ? strlen(modelNumber) : 0, /* minLength */
                                        (modelNumber != NULL) ? strlen(modelNumber) : 0, /* maxLength */
                                        GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

        GattCharacteristic serialNumberStringCharacteristic(GattCharacteristic::UUID_SERIAL_NUMBER_STRING_CHAR,
                                         (uint8_t *)serialNumber,
                                         (serialNumber != NULL) ? strlen(serialNumber) : 0, /* minLength */
                                         (serialNumber != NULL) ? strlen(serialNumber) : 0, /* maxLength */
                                         GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

        GattCharacteristic hardwareRevisionStringCharacteristic(GattCharacteristic::UUID_HARDWARE_REVISION_STRING_CHAR,
                                             (uint8_t *)hardwareRevision,
                                             (hardwareRevision != NULL) ? strlen(hardwareRevision) : 0, /* minLength */
                                             (hardwareRevision != NULL) ? strlen(hardwareRevision) : 0, /* maxLength */
                                             GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

        GattCharacteristic firmwareRevisionStringCharacteristic(GattCharacteristic::UUID_FIRMWARE_REVISION_STRING_CHAR,
                                             (uint8_t *)firmwareRevision,
                                             (firmwareRevision != NULL) ? strlen(firmwareRevision) : 0, /* minLength */
                                             (firmwareRevision != NULL) ? strlen(firmwareRevision) : 0, /* maxLength */
                                             GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

        GattCharacteristic softwareRevisionStringCharacteristic(GattCharacteristic::UUID_SOFTWARE_REVISION_STRING_CHAR,
                                             (uint8_t *)softwareRevision,
                                             (softwareRevision != NULL) ? strlen(softwareRevision) : 0, /* minLength */
                                             (softwareRevision != NULL) ? strlen(softwareRevision) : 0, /* maxLength */
                                             GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

        GattCharacteristic pnpIDCharacteristic(GattCharacteristic::UUID_PNP_ID_CHAR, (uint8_t *)PnPID);

        GattCharacteristic *charTable[] = {&manufacturersNameStringCharacteristic,
                                           &modelNumberStringCharacteristic,
                                           &serialNumberStringCharacteristic,
                                           &hardwareRevisionStringCharacteristic,
                                           &firmwareRevisionStringCharacteristic,
                                           &softwareRevisionStringCharacteristic,
                                           &pnpIDCharacteristic};

        GattService         deviceInformationService(GattService::UUID_DEVICE_INFORMATION_SERVICE, charTable,
                                                     sizeof(charTable) / sizeof(GattCharacteristic *));

        // for some reason, this causes the micro:bit build to break.
        // pnpIDCharacteristic.requireSecurity(SecurityManager::SECURITY_MODE_ENCRYPTION_NO_MITM);
        ble.addService(deviceInformationService);
        serviceAdded = true;
    }

protected:
    BLE                 &ble;
};

#endif /* #ifndef __BLE_HID_DEVICE_INFORMATION_SERVICE_H__*/
