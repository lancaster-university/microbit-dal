/* mbed Microcontroller Library
 * Copyright (c) 2015 ARM Limited
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
#ifndef HID_SERVICE_BASE_H_
#define HID_SERVICE_BASE_H_

#include "mbed.h"

#include "ble/BLE.h"
#include "HID_types.h"

#define BLE_UUID_DESCRIPTOR_REPORT_REFERENCE 0x2908

typedef const uint8_t report_map_t[];
typedef const uint8_t * report_t;

typedef struct {
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  flags;
} HID_information_t;

enum ReportType {
    INPUT_REPORT    = 0x1,
    OUTPUT_REPORT   = 0x2,
    FEATURE_REPORT  = 0x3,
};

enum ProtocolMode {
    BOOT_PROTOCOL   = 0x0,
    REPORT_PROTOCOL = 0x1,
};

typedef struct {
    uint8_t ID;
    uint8_t type;
} report_reference_t;


class HIDServiceBase {
public:
    /**
     *  Constructor
     *
     *  @param _ble
     *         BLE object to add this service to
     *  @param reportMap
     *         Byte array representing the input/output report formats. In USB HID jargon, it
     *         is called "HID report descriptor".
     *  @param reportMapLength
     *         Size of the reportMap array
     *  @param inputReport
     *         Input report
     *  @param inputReportLength
     *         Maximum length of a received report (up to 64 bytes) (default: 64 bytes)
     *  @param inputReportTickerDelay
     *         Delay between input report notifications, in ms. Acceptable values depend directly on
     *         GAP's connInterval parameter, so it shouldn't be less than 12ms
     *         Preferred GAP connection interval is set after this value, in order to send
     *         notifications as quick as possible: minimum connection interval will be set to
     *         (inputReportTickerDelay / 2)
     */
    HIDServiceBase(BLE &_ble,
                   report_map_t reportMap,
                   uint8_t reportMapLength,
                   report_t inputReport,
                   uint8_t inputReportLength = 0,
                   uint8_t inputReportTickerDelay = 50);

    /**
     *  Send Report
     *
     *  @param report   Report to send. Must be of size @ref inputReportLength
     *  @return         The write status
     *
     *  @note Don't call send() directly for multiple reports! Use reportTicker for that, in order
     *  to avoid overloading the BLE stack, and let it handle events between each report.
     */
    virtual ble_error_t send(const report_t report);

    /**
     *  Read Report
     *
     *  @param report   Report to fill. Must be of size @ref outputReportLength
     *  @return         The read status
     */
    virtual ble_error_t read(report_t report);

    virtual void onConnection(const Gap::ConnectionCallbackParams_t *params);
    virtual void onDisconnection(const Gap::DisconnectionCallbackParams_t *params);

    virtual bool isConnected(void)
    {
        return connected;
    }

protected:
    /**
     * Called by BLE API when data has been successfully sent.
     *
     * @param count     Number of reports sent
     *
     * @note Subclasses can override this to avoid starting the report ticker when there is nothing
     * to send
     */
    virtual void onDataSent(unsigned count);

    /**
     * Start the ticker that sends input reports at regular interval
     *
     * @note reportTickerIsActive describes the state of the ticker and can be used by HIDS
     * implementations.
     */
    virtual void startReportTicker(void);

    /**
     * Stop the input report ticker
     */
    virtual void stopReportTicker(void);

    /**
     * Called by input report ticker at regular interval (reportTickerDelay). This must be
     * overriden by HIDS implementations to call the @ref send() with a report, if necessary.
     */
    virtual void sendCallback(void) = 0;

    /**
     * Create the HID information structure
     */
    HID_information_t* HIDInformation();

protected:
    BLE &ble;
    bool connected;

    int reportMapLength;

    uint8_t inputReportLength;

    GattAttribute::Handle_t inputReportCharacteristicValueHandle;

    // Required gatt characteristics: Report Map, Information, Control Point
    GattCharacteristic reportMapCharacteristic;
    ReadOnlyGattCharacteristic<HID_information_t> HIDInformationCharacteristic;

    Ticker reportTicker;
    uint32_t reportTickerDelay;
    bool reportTickerIsActive;
};

#endif
