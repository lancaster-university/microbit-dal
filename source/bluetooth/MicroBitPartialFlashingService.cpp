
/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

/**
  * Class definition for the custom MicroBit Partial Flashing service.
  * Provides a BLE service to remotely write the user program to the device.
  */
#include "MicroBitConfig.h"
#include "ble/UUID.h"
#include "MicroBitPartialFlashingService.h"

/**
  * Constructor.
  * Create a representation of the PartialFlashService
  * @param _ble The instance of a BLE device that we're running on.
  * @param _messageBus The instance of a EventModel that we're running on.
  */
MicroBitPartialFlashingService::MicroBitPartialFlashingService(BLEDevice &_ble, EventModel &_messageBus) :
        ble(_ble), messageBus(_messageBus)
{
    // Set up partial flashing characteristic
    uint8_t initCharacteristicValue = 0x00;
    GattCharacteristic partialFlashCharacteristic(MicroBitPartialFlashingServiceCharacteristicUUID, &initCharacteristicValue, sizeof(initCharacteristicValue),
    20, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);

    // Set default security requirements
    partialFlashCharacteristic.requireSecurity(SecurityManager::MICROBIT_BLE_SECURITY_LEVEL);

    // Create Partial Flashing Service
    GattCharacteristic *characteristics[] = {&partialFlashCharacteristic};
    GattService         service(MicroBitPartialFlashingServiceUUID, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic*) );
    ble.addService(service);

    // Get characteristic handle for future use
    partialFlashCharacteristicHandle = partialFlashCharacteristic.getValueHandle();
    ble.gattServer().onDataWritten(this, &MicroBitPartialFlashingService::onDataWritten);

    // Set up listener for SD writing
    messageBus.listen(MICROBIT_ID_PARTIAL_FLASHING, MICROBIT_EVT_ANY, this, &MicroBitPartialFlashingService::partialFlashingEvent);

}


/**
  * Callback. Invoked when any of our attributes are written via BLE.
  */
void MicroBitPartialFlashingService::onDataWritten(const GattWriteCallbackParams *params)
{
    // Get data from BLE callback params
    uint8_t *data = (uint8_t *)params->data;

    if(params->handle == partialFlashCharacteristicHandle && params->len > 0)
    {
      // Switch CONTROL byte
      switch(data[0]){
        case REGION_INFO:
        {
          // Create instance of Memory Map to return info
          MicroBitMemoryMap memoryMap;

          uint8_t buffer[18];
          // Response:
          // Region and Region #
          buffer[0] =  0x00;
          buffer[1] =  data[1];

          // Start Address
          buffer[2] = (memoryMap.memoryMapStore.memoryMap[data[1]].startAddress & 0xFF000000) >> 24;
          buffer[3] = (memoryMap.memoryMapStore.memoryMap[data[1]].startAddress & 0x00FF0000) >> 16;
          buffer[4] = (memoryMap.memoryMapStore.memoryMap[data[1]].startAddress & 0x0000FF00) >>  8;
          buffer[5] = (memoryMap.memoryMapStore.memoryMap[data[1]].startAddress & 0x000000FF);

          // End Address
          buffer[6] = (memoryMap.memoryMapStore.memoryMap[data[1]].endAddress & 0xFF000000) >> 24;
          buffer[7] = (memoryMap.memoryMapStore.memoryMap[data[1]].endAddress & 0x00FF0000) >> 16;
          buffer[8] = (memoryMap.memoryMapStore.memoryMap[data[1]].endAddress & 0x0000FF00) >>  8;
          buffer[9] = (memoryMap.memoryMapStore.memoryMap[data[1]].endAddress & 0x000000FF);

          // Region Hash
          memcpy(&buffer[10], &memoryMap.memoryMapStore.memoryMap[data[1]].hash, 8);

          // Send BLE Notification
          ble.gattServer().notify(partialFlashCharacteristicHandle, (const uint8_t *)buffer, 18);

          // Reset packet count
          packetCount = 0;
          blockPacketCount = 0;
          blockNum = 0;
          offset = 0;

          break;
        }
        case FLASH_DATA:
        {
          // Process FLASH data packet
          flashData(data);
          break;
        }
        case END_OF_TRANSMISSION:
        {
          /* Start of embedded source isn't always on a page border so client must
           * inform the micro:bit that it's the last packet.
           * - Write final packet
           * The END_OF_TRANSMISSION packet contains no data. Write any data left in the buffer.
           */
           MicroBitEvent evt(MICROBIT_ID_PARTIAL_FLASHING, END_OF_TRANSMISSION);
           break;
        }
        case MICROBIT_STATUS:
        {
          /*
           * Return the version of the Partial Flashing Service and the current BLE mode (application / pairing)
           */
          uint8_t flashNotificationBuffer[] = {MICROBIT_STATUS, PARTIAL_FLASHING_VERSION, MicroBitBLEManager::manager->getCurrentMode()};
          ble.gattServer().notify(partialFlashCharacteristicHandle, (const uint8_t *)flashNotificationBuffer, sizeof(flashNotificationBuffer));
          break;
        }
        case MICROBIT_RESET:
        {
          /*
           * data[1] determines which mode to reset into: MICROBIT_MODE_PAIRING or MICROBIT_MODE_APPLICATION
           */
           switch(data[1]) {
             case MICROBIT_MODE_PAIRING:
             {
               MicroBitEvent evt(MICROBIT_ID_PARTIAL_FLASHING, MICROBIT_RESET );
               break;
             }
             case MICROBIT_MODE_APPLICATION:
             {
               microbit_reset();
               break;
             }
           }
           break;
        }
    }
  }
}



/**
  * @param data - A pointer to the data to process
  *
  */
void MicroBitPartialFlashingService::flashData(uint8_t *data)
{
        // Receive 16 bytes per packet
        // Buffer 4 packets
        // When buffer is full trigger partialFlashingEvent
        // When write is complete notify app and repeat
        // +-----------+---------+---------+----------+
        // | 1 Byte    | 2 Bytes | 1 Byte  | 16 Bytes |
        // +-----------+---------+---------+----------+
        // | COMMAND   | OFFSET  | PACKET# | DATA     |
        // +-----------+---------+---------+----------+
        uint8_t packetNum = data[3];

        /**
          * Check packet count
          * If the packet count doesn't match send a notification to the client
          * and set the packet count to the next block number
          */
        if (packetNum != packetCount)
        {
          if ( packetNum < packetCount ? packetCount - packetNum < 8 : packetNum - packetCount > 248 )
            return; // packet is from a previous batch

          uint8_t flashNotificationBuffer[] = {FLASH_DATA, 0xAA};
          ble.gattServer().notify(partialFlashCharacteristicHandle, (const uint8_t *)flashNotificationBuffer, sizeof(flashNotificationBuffer));
          blockPacketCount += 4;
          packetCount = blockPacketCount;
          blockNum = 0;
          return;
        }

        packetCount++;

        // Add to block
        memcpy(block + (4*blockNum), data + 4, 16);

        // Actions
        switch(blockNum) {
            // blockNum is 0: set up offset
            case 0:
                {
                    offset = ((data[1] << 8) | data[2] << 0);
                    blockNum++;
                    break;
                }
            // blockNum is 1: complete the offset
            case 1:
                {
                    offset |= ((data[1] << 24) | data[2] << 16);
                    blockNum++;
                    break;
                }
            // blockNum is 3, block is full
            case 3:
                {
                    // Fire write event
                    MicroBitEvent evt(MICROBIT_ID_PARTIAL_FLASHING, FLASH_DATA );
                    // Reset blockNum
                    blockNum = 0;
                    blockPacketCount += 4;
                    break;
                }
            default:
                {
                    blockNum++;
                    break;
                }
        }

}


/**
  * Write Event
  * Used the write data to the flash outside of the BLE ISR
  */
void MicroBitPartialFlashingService::partialFlashingEvent(MicroBitEvent e)
{
  MicroBitFlash flash;

  switch(e.value){
    case FLASH_DATA:
    {
      /*
       * Set flashIncomplete flag if not already set to boot into BLE mode
       * upon a failed flash.
       */
       MicroBitStorage storage;
       KeyValuePair* flashIncomplete = storage.get("flashIncomplete");
       if(flashIncomplete == NULL){
         uint8_t flashIncompleteVal = 0x01;
         storage.put("flashIncomplete", &flashIncompleteVal, sizeof(flashIncompleteVal));
       }
       delete flashIncomplete;

      uint32_t *flashPointer   = (uint32_t *)(offset);

      // If the pointer is on a page boundary erase the page
      if(!((uint32_t)flashPointer % 0x400))
          flash.erase_page(flashPointer);

      // Create a pointer to the data block
      uint32_t *blockPointer;
      blockPointer = block;

      // Write to flash
      flash.flash_burn(flashPointer, blockPointer, 16);

      // Update flash control buffer to send next packet
      uint8_t flashNotificationBuffer[] = {FLASH_DATA, 0xFF};
      ble.gattServer().notify(partialFlashCharacteristicHandle, (const uint8_t *)flashNotificationBuffer, sizeof(flashNotificationBuffer));
      break;
    }
    case END_OF_TRANSMISSION:
    {
      // Write final packet
      uint32_t *blockPointer;
      uint32_t *flashPointer   = (uint32_t *) offset;

      blockPointer = block;
      flash.flash_burn(flashPointer, blockPointer, 16);

      // Search for and remove embedded source magic (if it exists!)
      // Move to next page
      flashPointer = flashPointer + 0x400;

      // Iterate through until reaching the scratch page
      while(flashPointer < (uint32_t *)DEFAULT_SCRATCH_PAGE)
      {
        // Check for embedded source magic
        if(*flashPointer == 0x41140EF && *(uint32_t *)(flashPointer + 0x1) == 0xB82FA2BB)
        {
          // Embedded Source Found!
          uint8_t blank = 0x00;
          flash.flash_write(flashPointer, &blank, sizeof(blank));
        }

        // Next 16 byte alignment
        flashPointer = flashPointer + 0x2;
      }


      // Once the final packet has been written remove the BLE mode flag and reset
      // the micro:bit
        MicroBitStorage storage;
        storage.remove("flashIncomplete");
        microbit_reset();
      break;
    }
    case MICROBIT_RESET:
    {
      MicroBitBLEManager::manager->restartInBLEMode();
      break;
    }
  }

}

const uint8_t  MicroBitPartialFlashingServiceUUID[] = {
    0xe9,0x7d,0xd9,0x1d,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};

const uint8_t  MicroBitPartialFlashingServiceCharacteristicUUID[] = {
    0xe9,0x7d,0x3b,0x10,0x25,0x1d,0x47,0x0a,0xa0,0x62,0xfa,0x19,0x22,0xdf,0xa9,0xa8
};
