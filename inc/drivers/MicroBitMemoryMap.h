/*
The MIT License (MIT)

This class has been written by Sam Kent for the Microbit Educational Foundation.

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

#ifndef MICROBIT_MEMORY_MAP_H
#define MICROBIT_MEMORY_MAP_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "ManagedString.h"
#include "ErrorNo.h"

#define NUMBER_OF_REGIONS 3

/**
  * Class definition for the MicroBitMemoryMap class.
  * This allows reading and writing of regions within the memory map.
  *
  * This class maps the different regions used on the flash memory to allow
  * a region to updated independently of the others AKA Partial Flashing.
  */
class MicroBitMemoryMap
{
    struct Region
    {
        uint8_t  regionId;
        uint32_t startAddress;
        uint32_t endAddress;
        uint8_t  hash[8];

        Region(uint8_t regionId, uint32_t startAddress, uint32_t endAddress, uint8_t hash[8])
        {
            this->regionId = regionId;
            this->startAddress = startAddress;
            this->endAddress = endAddress;
            memcpy( this->hash, hash, 8 );
        }

        Region(){
          this->regionId = 0x0;
          this->startAddress = 0x0;
          this->endAddress = 0x0;
          memset( this->hash, 0x00, 8 );
        }

    };

    struct MemoryMapStore
    {
        Region memoryMap[NUMBER_OF_REGIONS];
    };

    uint8_t regionCount = 0;

    public:

    MemoryMapStore memoryMapStore;

    /**
      * Default constructor.
      *
      * Creates an instance of MicroBitMemoryMap
      */
    MicroBitMemoryMap();

    /**
     * Function for adding a Region to the end of the MemoryMap
     *
     * @param region The Region to add to the MemoryMap
     *
     * @return MICROBIT_OK on success
     *
     */
    int pushRegion(Region region);

    /**
      * Function for updating a Region of the MemoryMap
      *
      * @param region The Region to update in the MemoryMap. The name is used as the selector.
      *
      * @return MICROBIT_OK success, MICROBIT_NO_DATA if the region is not found
      */
    int updateRegion(Region region);

    /**
     * Function to fetch hashes from PXT build
     *
     */
    void findHashes();
};

#endif
