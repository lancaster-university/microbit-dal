#include "MicroBitKeyboardService.h"
#include "ble/UUID.h"
#include "ble/GattCharacteristic.h"
#include "MicroBitEvent.h"
#include "MicroBitFiber.h"
#include "MicroBitSystemTimer.h"
#include "NotifyEvents.h"
#include "ScanParametersService.h"
#include "BLE/Gap.h"

struct HIDReportReference{
    uint8_t id;
    uint8_t type;
};

struct HIDInformation{
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  flags;
};

#define BLE_UUID_DESCRIPTOR_REPORT_REFERENCE 0x2908

#define HID_INPUT_REPORT        0x01
#define HID_OUTPUT_REPORT       0x02
#define HID_FEATURE_REPORT      0x03

#define HID_REPORT_COUNT        4

const HIDReportReference reports[HID_REPORT_COUNT] = {
    {
        HID_INPUT_REPORT,
        HID_INPUT_REPORT
    },
    {
        HID_OUTPUT_REPORT,
        HID_OUTPUT_REPORT
    },
    {
        HID_FEATURE_REPORT,
        HID_FEATURE_REPORT
    },
    {
        0x2a,
        0x19
    }
};

const uint8_t reportMap[] =
// {
//     USAGE_PAGE(1), 0x01, // Generic Desktop Ctrls
//     USAGE(1), 0x06,      // Keyboard
//     COLLECTION(1), 0x01, // Application
//     USAGE_PAGE(1), 0x07, //   Kbrd/Keypad
//     USAGE_MINIMUM(1), 0xE0,
//     USAGE_MAXIMUM(1), 0xE7,
//     LOGICAL_MINIMUM(1), 0x00,
//     LOGICAL_MAXIMUM(1), 0x01,
//     REPORT_SIZE(1), 0x01, //   1 byte (Modifier)
//     REPORT_COUNT(1), 0x08,
//     INPUT(1), 0x02,        //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
//     REPORT_COUNT(1), 0x01, //   1 byte (Reserved)
//     REPORT_SIZE(1), 0x08,
//     INPUT(1), 0x01,        //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
//     REPORT_COUNT(1), 0x05, //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
//     REPORT_SIZE(1), 0x01,
//     USAGE_PAGE(1), 0x08,    //   LEDs
//     USAGE_MINIMUM(1), 0x01, //   Num Lock
//     USAGE_MAXIMUM(1), 0x05, //   Kana
//     OUTPUT(1), 0x02,        //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
//     REPORT_COUNT(1), 0x01,  //   3 bits (Padding)
//     REPORT_SIZE(1), 0x03,
//     OUTPUT(1), 0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
//     REPORT_COUNT(1), 0x06, //   6 bytes (Keys)
//     REPORT_SIZE(1), 0x08,
//     LOGICAL_MINIMUM(1), 0x00,
//     LOGICAL_MAXIMUM(1), 0x65, //   101 keys
//     USAGE_PAGE(1), 0x07,      //   Kbrd/Keypad
//     USAGE_MINIMUM(1), 0x00,
//     USAGE_MAXIMUM(1), 0x65,
//     INPUT(1), 0x00, //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
//     END_COLLECTION(0),
// };
// {

//     0x05, 0x01,                    // Usage Page (Generic Desktop)
//     0x09, 0x06,                    // Usage (Keyboard)
//     0xa1, 0x01,                    // Collection (Application)

//     // Modifier byte

//     0x75, 0x01,                    //   Report Size (1)
//     0x95, 0x08,                    //   Report Count (8)
//     0x05, 0x07,                    //   Usage Page (Key codes)
//     0x19, 0xe0,                    //   Usage Minimum (Keyboard LeftControl)
//     0x29, 0xe7,                    //   Usage Maxium (Keyboard Right GUI)
//     0x15, 0x00,                    //   Logical Minimum (0)
//     0x25, 0x01,                    //   Logical Maximum (1)
//     0x81, 0x02,                    //   Input (Data, Variable, Absolute)

//     // Reserved byte

//     0x75, 0x01,                    //   Report Size (1)
//     0x95, 0x08,                    //   Report Count (8)
//     0x81, 0x03,                    //   Input (Constant, Variable, Absolute)

//     // LED report + padding

//     0x95, 0x05,                    //   Report Count (5)
//     0x75, 0x01,                    //   Report Size (1)
//     0x05, 0x08,                    //   Usage Page (LEDs)
//     0x19, 0x01,                    //   Usage Minimum (Num Lock)
//     0x29, 0x05,                    //   Usage Maxium (Kana)
//     0x91, 0x02,                    //   Output (Data, Variable, Absolute)

//     0x95, 0x01,                    //   Report Count (1)
//     0x75, 0x03,                    //   Report Size (3)
//     0x91, 0x03,                    //   Output (Constant, Variable, Absolute)

//     // Keycodes

//     0x95, 0x06,                    //   Report Count (6)
//     0x75, 0x08,                    //   Report Size (8)
//     0x15, 0x00,                    //   Logical Minimum (0)
//     0x25, 0xff,                    //   Logical Maximum (1)
//     0x05, 0x07,                    //   Usage Page (Key codes)
//     0x19, 0x00,                    //   Usage Minimum (Reserved (no event indicated))
//     0x29, 0xff,                    //   Usage Maxium (Reserved)
//     0x81, 0x00,                    //   Input (Data, Array)

//     0xc0,                          // End collection
// };
{
    USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
    USAGE(1),           0x06,       // Keyboard
    COLLECTION(1),      0x01,       // Application
        REPORT_ID(1),       0x01,
        USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
        USAGE_MINIMUM(1),   0xE0,
        USAGE_MAXIMUM(1),   0xE7,
        LOGICAL_MINIMUM(1), 0x00,
        LOGICAL_MAXIMUM(1), 0x01,
        REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
        REPORT_COUNT(1),    0x08,
        INPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position

        REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
        REPORT_SIZE(1),     0x08,
        INPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position

        REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
        REPORT_SIZE(1),     0x01,
        USAGE_PAGE(1),      0x08,       //   LEDs
        USAGE_MINIMUM(1),   0x01,       //   Num Lock
        USAGE_MAXIMUM(1),   0x05,       //   Kana
        OUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile

        REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
        REPORT_SIZE(1),     0x03,
        OUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile

        REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
        REPORT_SIZE(1),     0x08,
        LOGICAL_MINIMUM(1), 0x00,
        LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
        USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
        USAGE_MINIMUM(1),   0x00,
        USAGE_MAXIMUM(1),   0x65,
        INPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position

        USAGE_PAGE(1),      0x0C,       //   Consumer
        USAGE(1),           0x00,
        COLLECTION(1),      0x02,       // Logical
            REPORT_ID(1),       0x02,
            USAGE_MINIMUM(1),   0x00,
            USAGE_MAXIMUM(1),   0xFF,
            REPORT_COUNT(1),    0x01,       //   1 byte
            REPORT_SIZE(1),     0x08,
            FEATURE(1),         0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
        END_COLLECTION(0),
    END_COLLECTION(0),

    USAGE_PAGE(1),      0x0C,
    USAGE(1),           0x01,
    COLLECTION(1),      0x01,
        REPORT_ID(1),   0x03,
        REPORT_SIZE(1), 0x10,
        REPORT_COUNT(1), 0x01,
        LOGICAL_MINIMUM(1), 1,
        LOGICAL_MAXIMUM(2), 0xFF, 0x03,
        USAGE_MINIMUM(1), 1,
        USAGE_MAXIMUM(2), 0xFF, 0x03,
        INPUT(1), 0x60,
    END_COLLECTION(0),
};
// {
//     USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
//     USAGE(1),           0x06,       // Keyboard
//     COLLECTION(1),      0x01,       // Application
//     USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
//     USAGE_MINIMUM(1),   0xE0,
//     USAGE_MAXIMUM(1),   0xE7,
//     LOGICAL_MINIMUM(1), 0x00,
//     LOGICAL_MAXIMUM(1), 0x01,
//     REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
//     REPORT_COUNT(1),    0x08,
//     INPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
//     REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
//     REPORT_SIZE(1),     0x08,
//     INPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
//     REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
//     REPORT_SIZE(1),     0x01,
//     USAGE_PAGE(1),      0x08,       //   LEDs
//     USAGE_MINIMUM(1),   0x01,       //   Num Lock
//     USAGE_MAXIMUM(1),   0x05,       //   Kana
//     OUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
//     REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
//     REPORT_SIZE(1),     0x03,
//     OUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
//     REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
//     REPORT_SIZE(1),     0x08,
//     LOGICAL_MINIMUM(1), 0x00,
//     LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
//     USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
//     USAGE_MINIMUM(1),   0x00,
//     USAGE_MAXIMUM(1),   0x65,
//     INPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
//     END_COLLECTION(0)
// };

static uint8_t inputReportData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static uint8_t controlPointCommand = 0;

static const HIDInformation HIDInfo[] = {0x0111, 0x00, 0x03};

/// "keys released" report
static const uint8_t emptyInputReportData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/// LEDs report
static uint8_t outputReportData = 0;

static uint8_t protocolMode =  1;

static const uint16_t uuid16_list[] = {GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE, GattService::UUID_DEVICE_INFORMATION_SERVICE, GattService::UUID_BATTERY_SERVICE};

/**
  * A simple helper function that "returns" our most recently "pressed" key to
  * the up position.
  */
void MicroBitKeyboardService::keyUp()
{
    ble.gattServer().write(keyboardInCharacteristic->getValueAttribute().getHandle(), emptyInputReportData, sizeof(emptyInputReportData));
    fiber_sleep(MICROBIT_HID_ADVERTISING_INT);
}

/**
  * Places and translates a single ascii character into a keyboard
  * value, placing it in our input buffer.
  */
int MicroBitKeyboardService::putc(const char c)
{
    if(!ble.getGapState().connected)
        return MICROBIT_NOT_SUPPORTED;

    inputReportData[0] = keymap[(uint8_t)c].modifier;
    inputReportData[2] = keymap[(uint8_t)c].usage;

    ble.gattServer().write(keyboardInCharacteristic->getValueAttribute().getHandle(), inputReportData, sizeof(inputReportData));
    fiber_sleep(MICROBIT_HID_ADVERTISING_INT);

    return 1;
}

/**
  * Constructor for our keyboard service.
  *
  * Creates a collection of characteristics, instantiates a battery service,
  * and modifies advertisement data.
  */
extern const uint8_t  MicroBitEventServiceUUID[];
MicroBitKeyboardService::MicroBitKeyboardService(BLEDevice& _ble, bool pairing)
: ble(_ble)
{
    status = 0;

    inputDescriptor = new GattAttribute(BLE_UUID_DESCRIPTOR_REPORT_REFERENCE,(uint8_t *)&reports[0], 2, 2, false);
    outputDescriptor = new GattAttribute(BLE_UUID_DESCRIPTOR_REPORT_REFERENCE,(uint8_t *)&reports[1], 2, 2, false);
    featureDescriptor = new GattAttribute(BLE_UUID_DESCRIPTOR_REPORT_REFERENCE,(uint8_t *)&reports[2], 2, 2, false);
    reportMapExternalRef = new GattAttribute(0x2907,(uint8_t *)&reports[3], 2, 2, false);

    GattAttribute* attrs[] =
    {
        (GattAttribute*)inputDescriptor,
        (GattAttribute*)outputDescriptor,
        (GattAttribute*)featureDescriptor,
        (GattAttribute*)reportMapExternalRef
    };

    // stack allocate our characteristics, except for the ones that may require notifications.
    // here we reuse our outputReportData array which is a single zero byte.
    protocolModeCharacteristic = new GattCharacteristic(GattCharacteristic::UUID_PROTOCOL_MODE_CHAR, (uint8_t *)&protocolMode, 1, 1, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE);
    keyboardInCharacteristic = new GattCharacteristic(GattCharacteristic::UUID_REPORT_CHAR, (uint8_t *)&inputReportData, sizeof(inputReportData), sizeof(inputReportData),GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE, &attrs[0], 1);
    GattCharacteristic keyboardOutCharacteristic(GattCharacteristic::UUID_REPORT_CHAR, (uint8_t *)&outputReportData, sizeof(outputReportData), sizeof(outputReportData), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE, &attrs[1], 1);
    GattCharacteristic featureReportCharacteristic(GattCharacteristic::UUID_REPORT_CHAR, (uint8_t *)&outputReportData, sizeof(outputReportData), sizeof(outputReportData), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE, &attrs[2], 1);
    bootInCharacteristic = new GattCharacteristic(GattCharacteristic::UUID_BOOT_KEYBOARD_INPUT_REPORT_CHAR, (uint8_t *)&inputReportData, sizeof(inputReportData), sizeof(inputReportData),GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);
    GattCharacteristic bootOutCharacteristic(GattCharacteristic::UUID_BOOT_KEYBOARD_OUTPUT_REPORT_CHAR, (uint8_t *)&outputReportData, sizeof(outputReportData), sizeof(outputReportData), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);
    GattCharacteristic reportMapCharacteristic(GattCharacteristic::UUID_REPORT_MAP_CHAR, (uint8_t *)&reportMap, sizeof(reportMap), sizeof(reportMap), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ, &attrs[3], 1);
    GattCharacteristic informationCharacteristic(GattCharacteristic::UUID_HID_INFORMATION_CHAR, (uint8_t *)&HIDInfo, sizeof(HIDInformation), sizeof(HIDInformation), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);
    controlPointCharacteristic = new GattCharacteristic(GattCharacteristic::UUID_HID_CONTROL_POINT_CHAR, (uint8_t *)&controlPointCommand, 1, 1, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE);

    GattCharacteristic* characteristics[] = {&reportMapCharacteristic, protocolModeCharacteristic, controlPointCharacteristic, &informationCharacteristic, keyboardInCharacteristic, &keyboardOutCharacteristic, &featureReportCharacteristic/*, bootInCharacteristic, &bootOutCharacteristic*/};
    GattService service(GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE/*MicroBitEventServiceUUID*/, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    protocolModeCharacteristic->requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    keyboardInCharacteristic->requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    keyboardOutCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    bootInCharacteristic->requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    bootOutCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    featureReportCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    reportMapCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    informationCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    controlPointCharacteristic->requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);

    inputDescriptorHandle = inputDescriptor->getHandle();
    outputDescriptorHandle = outputDescriptor->getHandle();
    featureDescriptorHandle = featureDescriptor->getHandle();
    pmCharacteristicHandle = protocolModeCharacteristic->getValueHandle();
    kInCharacteristicHandle = keyboardInCharacteristic->getValueHandle();
    kOutCharacteristicHandle = keyboardOutCharacteristic.getValueHandle();
    rMapCharacteristicHandle = reportMapCharacteristic.getValueHandle();
    infoCharacteristicHandle = informationCharacteristic.getValueHandle();
    cpCharacteristicHandle = controlPointCharacteristic->getValueHandle();

    batteryService = new BatteryService(ble);
    // paramsService = new ScanParametersService(ble);

    int ret = ble.addService(service);
    SERIAL_DEBUG->printf("ERR: %d\r\n",ret);


    ble.gattServer().onDataWritten(this,&MicroBitKeyboardService::debugWrite);
    ble.onDataWritten(this, &MicroBitKeyboardService::debugWrite);
    ble.gattServer().onDataRead(this,&MicroBitKeyboardService::debugRead);
    ble.onDataRead(this, &MicroBitKeyboardService::debugRead);

    ble.gattServer().write(bootInCharacteristic->getValueHandle(), (uint8_t *)&inputReportData, sizeof(inputReportData));

    ble.gap().stopAdvertising();
    ble.gap().clearAdvertisingPayload();

    /**< Minimum Connection Interval in 1.25 ms units, see BLE_GAP_CP_LIMITS.*/
	uint16_t minConnectionInterval = Gap::MSEC_TO_GAP_DURATION_UNITS(7.5);
	/**< Maximum Connection Interval in 1.25 ms units, see BLE_GAP_CP_LIMITS.*/
	uint16_t maxConnectionInterval = Gap::MSEC_TO_GAP_DURATION_UNITS(30);
	/**< Slave Latency in number of connection events, see BLE_GAP_CP_LIMITS.*/
	uint16_t slaveLatency = 6;
	/**< Connection Supervision Timeout in 10 ms units, see BLE_GAP_CP_LIMITS.*/
	uint16_t connectionSupervisionTimeout = 32 * 100;
	Gap::ConnectionParams_t connectionParams = {
		minConnectionInterval,
		maxConnectionInterval,
		slaveLatency,
		connectionSupervisionTimeout
	};

    ble.gap().setPreferredConnectionParams(&connectionParams);

    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::KEYBOARD);

    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS,(uint8_t *)&uuid16_list, sizeof(uuid16_list));

// #if CONFIG_ENABLED(MICROBIT_BLE_WHITELIST)
    if (!pairing)
        ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
// // #else
    else
        ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
// #endif
    // ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_LIMITED_DISCOVERABLE);

    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);

	// DEBUG_PRINTF_BLE("set advertising interval\r\n");
	ble.gap().setAdvertisingInterval(20);
	// DEBUG_PRINTF_BLE("set advertising timeout\r\n");
	ble.gap().setAdvertisingTimeout(30);

    ble.gap().accumulateAdvertisingPayload(
		GapAdvertisingData::COMPLETE_LOCAL_NAME,
		(uint8_t*)"poo", 3
	);

	// DEBUG_PRINTF_BLE("set device name\r\n");
    ble.gap().setDeviceName((uint8_t*)"poo");

	// ble.gap().setAdvertisingPolicyMode(Gap::ADV_POLICY_IGNORE_WHITELIST);
	// ble.gap().setScanningPolicyMode(Gap::SCAN_POLICY_FILTER_ALL_ADV);
	// ble.gap().setInitiatorPolicyMode(Gap::INIT_POLICY_FILTER_ALL_ADV);

    ble.gap().startAdvertising();
}

/**
  * Send a single character to our host.
  *
  * @param c the character to send
  *
  * @return MICROBIT_NOT_SUPPORTED if the micro:bit is not connected to a host.
  */
int MicroBitKeyboardService::send(const char c)
{
    if(status & MICROBIT_HID_STATE_IN_USE)
        fiber_wait_for_event(MICROBIT_ID_NOTIFY_ONE, MICROBIT_HID_SERVICE_FREE);

    status |= MICROBIT_HID_STATE_IN_USE;

    int ret = putc(c);
    keyUp();

    status &= ~MICROBIT_HID_STATE_IN_USE;
    MicroBitEvent(MICROBIT_ID_NOTIFY_ONE, MICROBIT_HID_SERVICE_FREE);

    return ret;
}

/**
  * Send a "Special" non-ascii keyboard key, defined in BluetoothHIDKeys.h
  */
int MicroBitKeyboardService::send(MediaKey)
{
    return MICROBIT_NOT_SUPPORTED;
}

/**
  * Send a buffer of characters to our host.
  *
  * @param data the characters to send
  *
  * @param len the number of characters in the buffer.
  *
  * @return MICROBIT_NOT_SUPPORTED if the micro:bit is not connected to a host.
  */
int MicroBitKeyboardService::send(const char* c, int len)
{
    if(status & MICROBIT_HID_STATE_IN_USE)
        fiber_wait_for_event(MICROBIT_ID_NOTIFY_ONE, MICROBIT_HID_SERVICE_FREE);

    status |= MICROBIT_HID_STATE_IN_USE;

    int sent = 0;
    int ret = 0;
    uint8_t previous = 0;

    for(int i = 0; i < len; i++)
    {
        // if our current key is the same as our previous key, we need to send an
        // up.
        if(previous == c[i])
            keyUp();

        if((ret = putc(c[i])) == MICROBIT_NOT_SUPPORTED)
            break;

        previous = c[i];
        sent += ret;
    }

    keyUp();

    status &= ~MICROBIT_HID_STATE_IN_USE;
    MicroBitEvent(MICROBIT_ID_NOTIFY_ONE, MICROBIT_HID_SERVICE_FREE);

    return sent;
}

/**
  * Send a ManagedString to our host.
  *
  * @param data the string to send
  *
  * @return MICROBIT_NOT_SUPPORTED if the micro:bit is not connected to a host.
  */
int MicroBitKeyboardService::send(ManagedString data)
{
    return send(data.toCharArray(), data.length());
}

void MicroBitKeyboardService::debugRead(const GattReadCallbackParams *params)
{
    SERIAL_DEBUG->printf("R: ");

    if (params->handle == pmCharacteristicHandle)
        SERIAL_DEBUG->printf("PM");

    if (params->handle == kInCharacteristicHandle)
        SERIAL_DEBUG->printf("KI");

    if (params->handle == kOutCharacteristicHandle)
        SERIAL_DEBUG->printf("KO");

    if (params->handle == rMapCharacteristicHandle)
        SERIAL_DEBUG->printf("RM");

    if (params->handle == infoCharacteristicHandle)
        SERIAL_DEBUG->printf("INF");

    if (params->handle == cpCharacteristicHandle)
        SERIAL_DEBUG->printf("CP");
}

void MicroBitKeyboardService::debugWrite(const GattWriteCallbackParams *params)
{
    SERIAL_DEBUG->printf("W: ");

    if (params->handle == inputDescriptorHandle)
        SERIAL_DEBUG->printf("INP");

    if (params->handle == outputDescriptorHandle)
        SERIAL_DEBUG->printf("OUT");

    if (params->handle == featureDescriptorHandle)
        SERIAL_DEBUG->printf("FE");

    if (params->handle == pmCharacteristicHandle)
        SERIAL_DEBUG->printf("PM");

    else if (params->handle == kInCharacteristicHandle)
        SERIAL_DEBUG->printf("KI");

    else if (params->handle == kOutCharacteristicHandle)
        SERIAL_DEBUG->printf("KO");

    else if (params->handle == rMapCharacteristicHandle)
        SERIAL_DEBUG->printf("RM");

    else if (params->handle == infoCharacteristicHandle)
        SERIAL_DEBUG->printf("INF");

    else if (params->handle == cpCharacteristicHandle)
        SERIAL_DEBUG->printf("CP");
    // else
    //     ble.gap().disconnect(Gap::LOCAL_HOST_TERMINATED_CONNECTION);
}

/**
  * Destructor.
  *
  * Frees our heap allocated characteristic and service, and remove this instance
  * from our system timer interrupt.
  */
MicroBitKeyboardService::~MicroBitKeyboardService()
{
    delete batteryService;
    delete keyboardInCharacteristic;
    delete inputDescriptor;
    delete outputDescriptor;
}
