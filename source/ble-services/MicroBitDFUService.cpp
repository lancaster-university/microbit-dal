/**
  * Class definition for a MicroBit Device Firmware Update loader.
  *
  * This is actually just a frontend to a memory resident nordic DFU loader.
  * Here we deal with the MicroBit 'pairing' functionality with BLE devices, and
  * very basic authentication and authorization. 
  *
  * This implementation is not intended to be fully secure, but rather intends to:
  *
  * 1. Provide a simple mechanism to identify an individual MicroBit amongst a classroom of others
  * 2. Allow BLE devices to discover and cache a passcode that can be used to flash the device over BLE.
  * 3. Provide a BLE escape route for programs that 'brick' the MicroBit. 
  *
  * Represents the device as a whole, and includes member variables to that reflect the components of the system.
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

    // Read/Write characteristic to enable unlocking and discovery of the MicroBit's flashcode.
    GattCharacteristic  microBitDFUServiceFlashCodeCharacteristic(MicroBitDFUServiceFlashCodeCharacteristicUUID, (uint8_t *)&flashCode, 0, sizeof(uint32_t),
    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    authenticated = false;
    flashCodeRequested = false;

    controlByte = 0x00;
    flashCode = 0x00;
    
    GattCharacteristic *characteristics[] = {&microBitDFUServiceControlCharacteristic, &microBitDFUServiceFlashCodeCharacteristic};
    GattService         service(MicroBitDFUServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

    ble.addService(service);

    microBitDFUServiceControlCharacteristicHandle = microBitDFUServiceControlCharacteristic.getValueHandle();
    microBitDFUServiceFlashCodeCharacteristicHandle = microBitDFUServiceFlashCodeCharacteristic.getValueHandle();

    ble.gattServer().onDataWritten(this, &MicroBitDFUService::onDataWritten);
}

void MicroBitDFUService::onButtonA(MicroBitEvent e)
{
    if (flashCodeRequested)
    {
        releaseFlashCode();
        uBit.display.scroll("");
        showTick();
        flashCodeRequested = false;
        authenticated = true;
    }
}

void MicroBitDFUService::onButtonB(MicroBitEvent e)
{
    uBit.display.scroll("VERSION: TODO");
    showNameHistogram();
}

/**
  * Begin the pairing process. Typically called when device is powered up with buttons held down.
  * Scroll a description on the display, then displays the device ID code as a histogram on the matrix display.
  */
void MicroBitDFUService::pair()
{
    uBit.MessageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_CLICK, this, &MicroBitDFUService::onButtonA);
    uBit.MessageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, this, &MicroBitDFUService::onButtonB);

    uBit.display.scroll("BLUE ZONE...");
    showNameHistogram();
    
    while(1)
    {    
        if (flashCodeRequested)
            uBit.display.scroll("PAIR?");
            
        // If our peer disconnects, drop all state.
        if ((authenticated || flashCodeRequested) && !ble.getGapState().connected)
        {
            authenticated = false;
            flashCodeRequested = false;
            flashCode = 0x00;
        }

        uBit.sleep(500);
    }
}


/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitDFUService::onDataWritten(const GattWriteCallbackParams *params)
{
    if (params->handle == microBitDFUServiceControlCharacteristicHandle)
    {
        if (params->len < 1)
            return;
        
        switch(params->data[0])
        {
            case MICROBIT_DFU_OPCODE_START_DFU:
            
                if (authenticated)
                {
                    uBit.display.scroll("");
                    uBit.display.clear();

#if CONFIG_ENABLED(MICROBIT_DBG)
                    uBit.serial.printf("  ACTIVATING BOOTLOADER.\n");
#endif
                    bootloader_start();    
                }   
            
                break;
            
            case MICROBIT_DFU_OPCODE_START_PAIR:
#if CONFIG_ENABLED(MICROBIT_DBG)
                uBit.serial.printf("  PAIRING REQUESTED.\n");
#endif
                flashCodeRequested = true;
                break;
                                
        }
    }
    
    if (params->handle == microBitDFUServiceFlashCodeCharacteristicHandle) 
    {
        if (params->len >= 4)
        {            
            uint32_t lockCode=0;
            memcpy(&lockCode, params->data, 4);
            if (lockCode == NRF_FICR->DEVICEID[0])
            {
#if CONFIG_ENABLED(MICROBIT_DBG)
                uBit.serial.printf("MicroBitDFU: FLASHCODE AUTHENTICATED\n");                
#endif
                authenticated = true;
            }else{
                authenticated = false;
            }
        }      
    }
}

/**
  * Displays the device's ID code as a histogram on the LED matrix display.
  */
void MicroBitDFUService::showTick()
{
    uBit.display.stopAnimation();
    
    uBit.display.image.setPixelValue(0,3, 255);
    uBit.display.image.setPixelValue(1,4, 255);
    uBit.display.image.setPixelValue(2,3, 255);
    uBit.display.image.setPixelValue(3,2, 255);
    uBit.display.image.setPixelValue(4,1, 255);
}


/**
  * Displays the device's ID code as a histogram on the LED matrix display.
  */
void MicroBitDFUService::showNameHistogram()
{
    uBit.display.stopAnimation();

    uint32_t n = NRF_FICR->DEVICEID[1];
    int ld = 1;
    int d = MICROBIT_DFU_HISTOGRAM_HEIGHT;
    int h;

    uBit.display.clear();
    for (int i=0; i<MICROBIT_DFU_HISTOGRAM_WIDTH;i++)
    {
        h = (n % d) / ld;

        n -= h;
        d *= MICROBIT_DFU_HISTOGRAM_HEIGHT;
        ld *= MICROBIT_DFU_HISTOGRAM_HEIGHT;

        for (int j=0; j<h+1; j++)
            uBit.display.image.setPixelValue(MICROBIT_DFU_HISTOGRAM_WIDTH-i-1, MICROBIT_DFU_HISTOGRAM_HEIGHT-j-1, 255);
    }       
}

/**
  * Displays the device's ID code as a histogram on the LED matrix display.
  */
void MicroBitDFUService::releaseFlashCode()
{
    flashCode = NRF_FICR->DEVICEID[0];

    ble.gattServer().notify(microBitDFUServiceFlashCodeCharacteristicHandle,(uint8_t *)&flashCode, sizeof(uint32_t));
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

const uint8_t              MicroBitDFUServiceFlashCodeCharacteristicUUID[] = {
    0xe9,0x5d,0x93,0xb2,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};
