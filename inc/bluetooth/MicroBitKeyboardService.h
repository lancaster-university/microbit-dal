/**
  * Implementation of a BLE Keyboard Service
  */

#ifndef MICROBIT_KEYBOARD_SERVICE_H
#define MICROBIT_KEYBOARD_SERVICE_H

#include "ble/BLE.h"
#include "KeyboardService.h"

#include "MicroBitEvent.h"
#include "MicroBitComponent.h"

#include "ManagedString.h"

enum ArrowKey {
    RIGHT = 0x94,
    LEFT,
    DOWN,
    UP
};

class MicroBitKeyboardService : public MicroBitComponent {
public:
    /**
     * Constructor
     * Create an instance of the Keyboard Service
     * @param _ble The instance of a BLE device that we're using.
     */
    MicroBitKeyboardService(BLEDevice &_ble);

    /**
     * Write a string using the Bluetooth keyboard interface
     */
    void writeString(ManagedString s);

    /**
     * Write a single character using the Bluetooth keyboard interface
     */
    void writeChar(char c);

    /**
     * Transmit an arrow key over Bluetooth
     */
    void writeArrowKey(ArrowKey k);

private:

    // BLE Device Instance
    BLEDevice &ble;

    // Keyboard Service Instance
    KeyboardService service;
};

#endif
