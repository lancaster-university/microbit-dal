#ifndef MICROBIT_KEYBOARD_SERVICE_H
#define MICROBIT_KEYBOARD_SERVICE_H

#include "hid/BluetoothHIDTypes.h"
#include "hid/BluetoothHIDKeys.h"
#include "ble/services/BatteryService.h"
#include "ManagedString.h"
#include "ble/BLE.h"

#include "MicroBitComponent.h"

#define MICROBIT_HID_S_EVT_TX_EMPTY     1
#define MICROBIT_HID_ADVERTISING_INT    24

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

    GattCharacteristic* keyboardInCharacteristic;

    /**
      * A simple helper function that "returns" our most recently "pressed" key to
      * the up position.
      */
    void keyUp();

    public:

    /**
      * Constructor for our keyboard service.
      *
      * Creates a collection of characteristics, instantiates a battery service,
      * and modifies advertisement data.
      */
    MicroBitKeyboardService(BLEDevice& _ble);

    /**
      * System tick is used to time the visibility of characters from the HID device.
      *
      * Our HID advertising interval is 24 ms, which means there will be a character
      * swap every 24 ms. Our system tick timer interrupt occures every 6ms...
      *
      * After we swap characters, we reset our counter, and emit an event to wake any
      * waiting fibers.
      */
    virtual void systemTick();

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

    /**
      * Destructor.
      *
      * Frees our heap allocated characteristic and service, and remove this instance
      * from our system timer interrupt.
      */
    ~MicroBitKeyboardService();
};

#endif
