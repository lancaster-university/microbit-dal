/**
  * Implementation of a BLE Keyboard Service
  */

#include "MicroBitKeyboardService.h"

#include "ble/BLE.h"

MicroBitKeyboardService::MicroBitKeyboardService(BLEDevice &_ble) : ble(_ble), service(_ble) {
    //ble.accumulateAdvertisingPayload(GapAdvertisingData::KEYBOARD);
}

void MicroBitKeyboardService::writeString(ManagedString s) {
    for (uint16_t i = 0; i < s.length(); i++) {
        service.putc(s.charAt(i));
    }
}

void MicroBitKeyboardService::writeChar(char c) {
    service.putc(c);
}

void MicroBitKeyboardService::writeArrowKey(ArrowKey k) {
    // The KeyboardService key map allows us to send arrow keys as 'special' ASCII characters
    service.putc(k);
}
