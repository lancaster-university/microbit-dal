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
  * Class definition for the MicroBitMemoryMap class.
  * This allows reading and writing of regions within the memory map.
  *
  * This class maps the different regions used on the flash memory to allow
  * a region to updated independently of the others AKA Partial Flashing.
*/

#include "MicroBitConfig.h"
#include "MicroBitMemoryMap.h"
#include "MicroBitFlash.h"
#include "MicroBitSerial.h"

/**
  * Default constructor.
  *
  * Creates an instance of MicroBitMemoryMap
  */
MicroBitMemoryMap::MicroBitMemoryMap()
{
      // Assumes PXT Built program
      // SD
      pushRegion(Region(0x00, 0x00, 0x18000, 0x00));  // Soft Device

      // DAL
      pushRegion(Region(0x01, 0x18000, FLASH_PROGRAM_END, 0x00)); // micro:bit Device Abstractation Layer

      // PXT
      // PXT will be on the start of the next page
      // padding to next page = 0x400 - (FLASH_PROGRAM_END % 0x400);
      pushRegion(Region(0x02, FLASH_PROGRAM_END + (0x400 - (FLASH_PROGRAM_END % 0x400)), 0x3BBFF, 0x00)); // micro:bit PXT

      // Find Hashes if PXT Built Program
      findHashes();

}

/**
  * Function for adding a Region to the end of the MemoryMap
  *
  * @param region The Region to add to the MemoryMap
  *
  * @return MICROBIT_OK on success
  *
  */
int MicroBitMemoryMap::pushRegion(Region region)
{

    // Find next blank Region in map
    if(regionCount == NUMBER_OF_REGIONS){
        return MICROBIT_NO_RESOURCES;
    } else {
        // Add data
        memoryMapStore.memoryMap[regionCount].startAddress = region.startAddress;
        memoryMapStore.memoryMap[regionCount].endAddress   = region.endAddress;
        //memcpy(&memoryMapStore.memoryMap[regionCount].hash, &region.hash, 8);
        memoryMapStore.memoryMap[regionCount++].regionId     = region.regionId;
        return MICROBIT_OK;
    }
}

/**
  * Function for updating a Region of the MemoryMap
  *
  * @param region The Region to update in the MemoryMap. The name is used as the selector.
  *
  * @return MICROBIT_OK success, MICROBIT_NO_RESOURCES if the region is not found
  */
int MicroBitMemoryMap::updateRegion(Region region)
{
    // Find Region name in map
    int i = 0;
    while(memoryMapStore.memoryMap[i].regionId != region.regionId && i < NUMBER_OF_REGIONS) i++;

    if(i == NUMBER_OF_REGIONS){
        return MICROBIT_NO_RESOURCES;
    } else {
        // Add data
        memoryMapStore.memoryMap[i] = region;
        return MICROBIT_OK;
    }
}

/*
 * Function to fetch the hashes from a PXT generated build
 */
void MicroBitMemoryMap::findHashes()
{
    // Iterate through pages to find magic
    uint32_t add = FLASH_PROGRAM_END;
    uint32_t rem = add % PAGE_SIZE;
    if ( rem) add += PAGE_SIZE - rem;
    int step = PAGE_SIZE / sizeof( uint32_t);
    uint32_t *nxt = ( uint32_t *)DEFAULT_SCRATCH_PAGE;
    for( uint32_t *magicAddress = (uint32_t *) add; magicAddress < nxt; magicAddress += step)
    {
        // Check for 16 bytes of Magic
        if (   magicAddress[0] == 0x923b8e70
            && magicAddress[1] == 0x41A815C6
            && magicAddress[2] == 0xC96698C4
            && magicAddress[3] == 0x9751EE75) {
                // If the magic has been found use the hashes follow
                memcpy( memoryMapStore.memoryMap[1].hash, magicAddress + 4, 8);
                memcpy( memoryMapStore.memoryMap[2].hash, magicAddress + 6, 8);
                memoryMapStore.memoryMap[2].startAddress = (uint32_t)magicAddress;
                return;
        }
    }
}
