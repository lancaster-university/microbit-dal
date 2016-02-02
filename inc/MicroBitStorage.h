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
