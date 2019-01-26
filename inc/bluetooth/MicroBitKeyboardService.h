#ifndef MICROBIT_KEYBOARD_SERVICE_H
#define MICROBIT_KEYBOARD_SERVICE_H

#include "hid/BluetoothHIDTypes.h"
#include "hid/BluetoothHIDKeys.h"
#include "ble/services/BatteryService.h"
#include "ManagedString.h"
#include "ble/BLE.h"
#include "ScanParametersService.h"

#include "MicroBitComponent.h"

#define MICROBIT_HID_ADVERTISING_INT    100
#define MICROBIT_HID_STATE_IN_USE       0x02

/**
  * This class represents a Bluetooth HID device, specifically, a keyboard instance.
  *
  * A few things to note:
  *     - HID devices require a battery service (automatically instantiated with this class)
  *     - Security is required, this class has only been tested using Just Works pairing.
  *     - On mac to get it to pair, you may have to interrogate a secure characteristic of the micro:bit via LightBlue
  *       in order to initiate pairing.
  *     - It is designed to be as light weight as possible, and employs tactics like the stack allocation of
  *       gatt characteristics to alleviate RAM pressures.
  */
class MicroBitKeyboardService : public MicroBitComponent
{
    private:
    BLEDevice& ble;
    BatteryService* batteryService;
    ScanParametersService* paramsService;

    uint16_t inputDescriptorHandle;
    uint16_t outputDescriptorHandle;
    uint16_t featureDescriptorHandle;
    uint16_t pmCharacteristicHandle;
    uint16_t kInCharacteristicHandle;
    uint16_t kOutCharacteristicHandle;
    uint16_t rMapCharacteristicHandle;
    uint16_t infoCharacteristicHandle;
    uint16_t cpCharacteristicHandle;

    GattAttribute* inputDescriptor;
    GattAttribute* outputDescriptor;
    GattAttribute* featureDescriptor;
    GattAttribute* reportMapExternalRef;

    GattCharacteristic* protocolModeCharacteristic;
    GattCharacteristic* controlPointCharacteristic;
    GattCharacteristic* keyboardInCharacteristic;
    GattCharacteristic* bootInCharacteristic;

    /**
      * A simple helper function that "returns" our most recently "pressed" key to
      * the up position.
      */
    void keyUp();

    /**
      * Places and translates a single ascii character into a keyboard
      * value, placing it in our input buffer.
      */
    int putc(const char c);

    public:

    /**
      * Constructor for our keyboard service.
      *
      * Creates a collection of characteristics, instantiates a battery service,
      * and modifies advertisement data.
      */
    MicroBitKeyboardService(BLEDevice& _ble, bool pairing);

    /**
      * Send a "Special" non-ascii keyboard key, defined in BluetoothHIDKeys.h
      */
    int send(MediaKey key);

    /**
      * Send a single character to our host.
      *
      * @param c the character to send
      *
      * @return MICROBIT_NOT_SUPPORTED if the micro:bit is not connected to a host.
      */
    int send(const char c);

    /**
      * Send a buffer of characters to our host.
      *
      * @param data the characters to send
      *
      * @param len the number of characters in the buffer.
      *
      * @return MICROBIT_NOT_SUPPORTED if the micro:bit is not connected to a host.
      */
    int send(const char* data, int len);

    /**
      * Send a ManagedString to our host.
      *
      * @param data the string to send
      *
      * @return MICROBIT_NOT_SUPPORTED if the micro:bit is not connected to a host.
      */
    int send(ManagedString data);

    void debugRead(const GattReadCallbackParams *params);

    void debugWrite(const GattWriteCallbackParams *params);

    /**
      * Destructor.
      *
      * Frees our heap allocated characteristic and service, and remove this instance
      * from our system timer interrupt.
      */
    ~MicroBitKeyboardService();
};

#endif
