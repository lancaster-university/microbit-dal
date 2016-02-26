/**
 * Class definition for a MicroBit Device Firmware Update loader.
 *
 * This is actually just a frontend to a memory resident nordic DFU loader.
 *
 * We rely on the BLE standard pairing processes to provide encryption and authentication.
 * We assume any device that is paied with the micro:bit is authorized to reprogram the device.
 *
 */

#include "MicroBit.h"
#include "ble/UUID.h"

#if !defined(__arm)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/*
 * The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
extern "C" {
#include "dfu_app_handler.h"
}

/*
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif


/**
 * Constructor.
 * Create a representation of a MicroBit device.
 * @param messageBus callback function to receive MicroBitMessageBus events.
 */
MicroBitDFUService::MicroBitDFUService(BLEDevice &_ble) :
    ble(_ble)
{
    // Opcodes can be issued here to control the MicroBitDFU Service, as defined above.
    GattCharacteristic  microBitDFUServiceControlCharacteristic(MicroBitDFUServiceControlCharacteristicUUID, &controlByte, 0, sizeof(uint8_t),
            GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);

    controlByte = 0x00;

    // Set default security requirements
    microBitDFUServiceControlCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);

    GattCharacteristic *characteristics[] = {&microBitDFUServiceControlCharacteristic};
    GattService         service(MicroBitDFUServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    microBitDFUServiceControlCharacteristicHandle = microBitDFUServiceControlCharacteristic.getValueHandle();

    ble.gattServer().write(microBitDFUServiceControlCharacteristicHandle, &controlByte, sizeof(uint8_t));
    ble.gattServer().onDataWritten(this, &MicroBitDFUService::onDataWritten);
}

/**
 * Callback. Invoked when any of our attributes are written via BLE.
 */
void MicroBitDFUService::onDataWritten(const GattWriteCallbackParams *params)
{
    if (params->handle == microBitDFUServiceControlCharacteristicHandle)
    {
        if(params->len > 0 && params->data[0] == MICROBIT_DFU_OPCODE_START_DFU)
        {
            uBit.display.stopAnimation();
            uBit.display.clear();

#if CONFIG_ENABLED(MICROBIT_DBG)
            uBit.serial.printf("  ACTIVATING BOOTLOADER.\n");
#endif

            // Perform an explicit disconnection to assist our peer to reconnect to the DFU service
            ble.disconnect(Gap::REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF);

            wait_ms(1000);

            // Call bootloader_start implicitly trough a event handler call
            // it is a work around for bootloader_start not being public in sdk 8.1
            ble_dfu_t p_dfu;
            ble_dfu_evt_t p_evt;

            p_dfu.conn_handle = params->connHandle;
            p_evt.ble_dfu_evt_type = BLE_DFU_START;

            dfu_app_on_dfu_evt(&p_dfu, &p_evt);
        }
    }
}


/**
 * UUID definitions for BLE Services and Characteristics.
 */

const uint8_t              MicroBitDFUServiceUUID[] = {
    0xe9,0x5d,0x93,0xb0,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t              MicroBitDFUServiceControlCharacteristicUUID[] = {
    0xe9,0x5d,0x93,0xb1,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};
