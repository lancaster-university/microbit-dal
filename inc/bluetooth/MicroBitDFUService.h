#ifndef MICROBIT_DFU_SERVICE_H
#define MICROBIT_DFU_SERVICE_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "ble/BLE.h"
#include "MicroBitEvent.h"

// MicroBit ControlPoint OpCodes
// Requests transfer to the Nordic DFU bootloader.
#define MICROBIT_DFU_OPCODE_START_DFU       1

// visual ID code constants
#define MICROBIT_DFU_HISTOGRAM_WIDTH        5
#define MICROBIT_DFU_HISTOGRAM_HEIGHT       5

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitDFUServiceUUID[];
extern const uint8_t  MicroBitDFUServiceControlCharacteristicUUID[];
extern const uint8_t  MicroBitDFUServiceFlashCodeCharacteristicUUID[];

// Handle on the memory resident Nordic bootloader.
extern "C" void bootloader_start(void);

/**
  * Class definition for a MicroBit Device Firmware Update loader.
  * This service allows hexes to be flashed remotely from another Bluetooth
  * device.
  */
class MicroBitDFUService
{
    public:

    /**
      * Constructor.
      * Initialise the Device Firmware Update service.
      * @param _ble The instance of a BLE device that we're running on.
      */
    MicroBitDFUService(BLEDevice &_ble);

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    virtual void onDataWritten(const GattWriteCallbackParams *params);

    private:

    // State of paiting process.
    bool authenticated;
    bool flashCodeRequested;

    // Bluetooth stack we're running on.
    BLEDevice           &ble;

    // memory for our 8 bit control characteristic.
    uint8_t             controlByte;

    // BLE pairing name of this device, encoded as an integer.
    uint32_t            flashCode;

    GattAttribute::Handle_t microBitDFUServiceControlCharacteristicHandle;
    GattAttribute::Handle_t microBitDFUServiceFlashCodeCharacteristicHandle;

    // Displays the device's ID code as a histogram on the LED matrix display.
    void showNameHistogram();

    // Displays an acknowledgement on the LED matrix display.
    void showTick();

    // Update BLE characteristic to release our flash code.
    void releaseFlashCode();

    // Event handlers for button clicks.
    void onButtonA(MicroBitEvent e);
    void onButtonB(MicroBitEvent e);
};

#endif
