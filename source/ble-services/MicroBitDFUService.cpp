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

/**
  * Constructor. 
  * Create a representation of a MicroBit device.
  * @param messageBus callback function to receive MicroBitMessageBus events.
  */
MicroBitDFUService::MicroBitDFUService(BLEDevice &_ble) : 
        ble(_ble) 
{
    // Opcodes can be issued here to control the MicroBitDFU Service, as defined above.
    WriteOnlyGattCharacteristic<uint8_t> microBitDFUServiceControlCharacteristic(MicroBitDFUServiceControlCharacteristicUUID, &controlByte);

    controlByte = 0x00;
    
    // Set default security requirements
    microBitDFUServiceControlCharacteristic.requireSecurity(SecurityManager::SECURITY_MODE_ENCRYPTION_WITH_MITM);

    GattCharacteristic *characteristics[] = {&microBitDFUServiceControlCharacteristic};
    GattService         service(MicroBitDFUServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    microBitDFUServiceControlCharacteristicHandle = microBitDFUServiceControlCharacteristic.getValueHandle();

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
            bootloader_start();    
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

