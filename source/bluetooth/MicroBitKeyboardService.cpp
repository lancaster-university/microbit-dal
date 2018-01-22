#include "MicroBitKeyboardService.h"
#include "ble/UUID.h"
#include "ble/GattCharacteristic.h"
#include "MicroBitEvent.h"
#include "MicroBitFiber.h"
#include "MicroBitSystemTimer.h"
#include "NotifyEvents.h"

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

#define HID_REPORT_COUNT        3

const HIDReportReference reports[HID_REPORT_COUNT] = {
    {
        0,
        HID_INPUT_REPORT
    },
    {
        1,
        HID_OUTPUT_REPORT
    },
    {
        2,
        HID_FEATURE_REPORT
    },
};

const uint8_t reportMap[] = {
    USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
    USAGE(1),           0x06,       // Keyboard
    COLLECTION(1),      0x01,       // Application
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
    END_COLLECTION(0)
};

static uint8_t inputReportData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static uint8_t controlPointCommand = 0;

static const HIDInformation HIDInfo[] = {0x0111, 0x00, 0x03};

/// "keys released" report
static const uint8_t emptyInputReportData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/// LEDs report
static uint8_t outputReportData[] = { 0 };

static const uint16_t uuid16_list[] = {GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE};

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
MicroBitKeyboardService::MicroBitKeyboardService(BLEDevice& _ble)
: ble(_ble)
{
    status = 0;

    GattAttribute inputDescriptor(BLE_UUID_DESCRIPTOR_REPORT_REFERENCE,(uint8_t *)&reports[0], 2, 2);
    GattAttribute outputDescriptor(BLE_UUID_DESCRIPTOR_REPORT_REFERENCE,(uint8_t *)&reports[1], 2, 2);

    GattAttribute* attrs[] = {
        (GattAttribute*)&inputDescriptor,
        (GattAttribute*)&outputDescriptor
    };

    // stack allocate our characteristics, except for the ones that may require notifications.
    // here we reuse our outputReportData array which is a single zero byte.
    GattCharacteristic protocolModeCharacteristic(GattCharacteristic::UUID_PROTOCOL_MODE_CHAR, (uint8_t *)&outputReportData, 1, 1, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE);
    keyboardInCharacteristic = new GattCharacteristic(GattCharacteristic::UUID_REPORT_CHAR, (uint8_t *)&inputReportData, sizeof(inputReportData), sizeof(inputReportData),GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE, &attrs[0], 1);
    GattCharacteristic keyboardOutCharacteristic(GattCharacteristic::UUID_REPORT_CHAR, (uint8_t *)&outputReportData, sizeof(outputReportData), sizeof(outputReportData), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE, &attrs[1], 1);
    GattCharacteristic reportMapCharacteristic(GattCharacteristic::UUID_REPORT_MAP_CHAR, (uint8_t *)&reportMap, sizeof(reportMap), sizeof(reportMap), GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);
    GattCharacteristic informationCharacteristic(GattCharacteristic::UUID_HID_INFORMATION_CHAR, (uint8_t *)&HIDInfo, sizeof(HIDInformation), sizeof(HIDInformation));
    GattCharacteristic controlPointCharacteristic(GattCharacteristic::UUID_HID_CONTROL_POINT_CHAR, (uint8_t *)&controlPointCommand, 1, 1, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE);

    GattCharacteristic* characteristics[] = {&informationCharacteristic, &reportMapCharacteristic, &protocolModeCharacteristic, &controlPointCharacteristic, keyboardInCharacteristic, &keyboardOutCharacteristic};

    protocolModeCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    keyboardInCharacteristic->requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    keyboardOutCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    reportMapCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    informationCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);
    controlPointCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);

    GattService service(GattService::UUID_HUMAN_INTERFACE_DEVICE_SERVICE, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    batteryService = new BatteryService(ble);

    ble.addService(service);

    ble.gattServer().write(keyboardInCharacteristic->getValueHandle(), (uint8_t *)&inputReportData, sizeof(inputReportData));

    ble.gap().stopAdvertising();
    ble.gap().clearAdvertisingPayload();
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::KEYBOARD);

    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS,(uint8_t *)&uuid16_list, sizeof(uuid16_list));
    ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);

    ble.gap().setAdvertisingInterval(MICROBIT_HID_ADVERTISING_INT);
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

/**
  * Destructor.
  *
  * Frees our heap allocated characteristic and service, and remove this instance
  * from our system timer interrupt.
  */
MicroBitKeyboardService::~MicroBitKeyboardService()
{
    free(batteryService);
    free(keyboardInCharacteristic);
}
