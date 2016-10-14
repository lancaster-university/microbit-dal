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

#ifndef MICROBIT_STORAGE_H
#define MICROBIT_STORAGE_H

#include "mbed.h"
#include "MicroBitBLEManager.h"
#include "MicroBitCompass.h"

#define MICROBIT_STORAGE_CONFIG_MAGIC       0xCAFECAFE

struct BLESysAttribute
{
    uint32_t        magic;
    uint8_t         sys_attr[8];
};

struct MicroBitConfigurationBlock
{
    uint32_t            magic;
    BLESysAttribute     sysAttrs[MICROBIT_BLE_MAXIMUM_BONDS];
    CompassSample       compassCalibrationData;
    int                 thermometerCalibration;
    int                 accessibility;
};


/**
  * Class definition for the MicroBitStorage class.
  * This allows reading and writing of small blocks of data to FLASH memory.
  */
class MicroBitStorage
{
    public:

    /*
     * Default constructor.
     */  
    MicroBitStorage();  
  
    /*
     * Writes the given number of bytes to the address specified.
     * @param buffer the data to write.
     * @param address the location in memory to write to.
     * @param length the number of bytes to write.
     * TODO: Write this one!
     */  
    int writeBytes(uint8_t *buffer, uint32_t address, int length);

    /** 
     * Method for erasing a page in flash.
     * @param page_address Address of the first word in the page to be erased.
     */
    void flashPageErase(uint32_t * page_address);

    /** 
     * Method for writing a word of data in flash with a value.
     
     * @param address Address of the word to change.
     * @param value Value to be written to flash.
     */
    void flashWordWrite(uint32_t * address, uint32_t value);

    /*
     * Reads the micro:bit's configuration data block from FLASH into a RAM buffer.
     * @return a structure containing the stored data. 
     */  
    MicroBitConfigurationBlock *getConfigurationBlock();

    /*
     * Writes the micro:bit's configuration data block from FLASH into a RAM buffer.
     * @return a structure containing the stored data. 
     */  
    int setConfigurationBlock(MicroBitConfigurationBlock *block);
};

#endif
