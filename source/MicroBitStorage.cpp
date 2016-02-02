/**
  * Class definition for the MicroBitStorage class.
  * This allows reading and writing of FLASH memory.
  */

#include "MicroBit.h"

/*
 * Default constructor
 */
MicroBitStorage::MicroBitStorage()
{
}

/*
 * Writes the given number of bytes to the address specified.
 * TODO: Complete this function to provide an abstraction across SD and no SD builds.
 *
 * @param buffer the data to write.
 * @param address the location in memory to write to.
 * @param length the number of bytes to write.
 */  
int MicroBitStorage::writeBytes(uint8_t *buffer, uint32_t address, int length)
{
    (void) buffer;
    (void) address;
    (void) length;
    
    return MICROBIT_OK;
}

/** 
 * Method for erasing a page in flash.
 *
 * @param page_address Address of the first word in the page to be erased.
 */
void MicroBitStorage::flashPageErase(uint32_t * page_address)
{
    // Turn on flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

    // Erase page:
    NRF_NVMC->ERASEPAGE = (uint32_t)page_address;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

    // Turn off flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
}


/*
 * Reads the micro:bit's configuration data block from FLASH into a RAM buffer.
 * @return a pointer to the structure containing the stored data. 
 * NOTE: it is the callers responsibility to free the buffer.
 */  

MicroBitConfigurationBlock *MicroBitStorage::getConfigurationBlock()
{
    uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
    uint32_t pg_num  = NRF_FICR->CODESIZE - 19;          // Use the page just below the BLE Bond Data 

    MicroBitConfigurationBlock *block = new MicroBitConfigurationBlock();
    memcpy(block, (uint32_t *)(pg_size * pg_num), sizeof(MicroBitConfigurationBlock));

    if (block->magic != MICROBIT_STORAGE_CONFIG_MAGIC) 
        memclr(block, sizeof(MicroBitConfigurationBlock));

#if CONFIG_ENABLED(MICROBIT_DBG)

    uBit.serial.printf("RETREIVE:\r\n");

    if(block->magic == MICROBIT_STORAGE_CONFIG_MAGIC)
    {
        uBit.serial.printf("magic: %.2x\r\n", block->magic);

        for(int attrIterator = 0; attrIterator < MICROBIT_BLE_MAXIMUM_BONDS; attrIterator++)
        {
            if(block->sysAttrs[attrIterator].magic == MICROBIT_STORAGE_CONFIG_MAGIC)
            {
                uBit.serial.printf("systemAttrs[%d]: ", attrIterator);

                for(int i = 0; i < 8; i++)
                {
                    uBit.serial.printf("%.2x\r\n", block->sysAttrs[attrIterator].sys_attr[i]);
                }

                uBit.serial.printf("\r\n");
            }
        }

        uBit.serial.printf("compass x: %d y: %d z: %d\r\n", block->compassCalibrationData.x, block->compassCalibrationData.y, block->compassCalibrationData.z);

        uBit.serial.printf("temperature: %d\r\n", block->thermometerCalibration);
    }
#endif

    return block;
}

/** 
 * Function for filling a page in flash with a value.
 *
 * @param address Address of the first word in the page to be filled.
 * @param value Value to be written to flash.
 */
void MicroBitStorage::flashWordWrite(uint32_t * address, uint32_t value)
{
    // Turn on flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

    *address = value;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);

    // Turn off flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
}


/*
 * Writes the micro:bit's configuration data block from FLASH into a RAM buffer.
 * @return a structure containing the stored data. 
 */  
int MicroBitStorage::setConfigurationBlock(MicroBitConfigurationBlock *block)
{

    #if CONFIG_ENABLED(MICROBIT_DBG)

        uBit.serial.printf("STORE:\r\n");

        if(block->magic == MICROBIT_STORAGE_CONFIG_MAGIC)
        {
            uBit.serial.printf("magic: %.2x\r\n", block->magic);

            for(int attrIterator = 0; attrIterator < MICROBIT_BLE_MAXIMUM_BONDS; attrIterator++)
            {
                if(block->sysAttrs[attrIterator].magic == MICROBIT_STORAGE_CONFIG_MAGIC)
                {
                    uBit.serial.printf("systemAttrs[%d]: ", attrIterator);

                    for(int i = 0; i < 8; i++)
                    {
                        uBit.serial.printf("%.2x\r\n", block->sysAttrs[attrIterator].sys_attr[i]);
                    }

                    uBit.serial.printf("\r\n");
                }
            }

            uBit.serial.printf("compass x: %d y: %d z: %d\r\n", block->compassCalibrationData.x, block->compassCalibrationData.y, block->compassCalibrationData.z);

            uBit.serial.printf("temperature: %d\r\n", block->thermometerCalibration);
        }
    #endif


    uint32_t * addr;
    uint32_t   pg_size;
    uint32_t   pg_num;
    int   wordsToWrite = sizeof(MicroBitConfigurationBlock) / 4 + 1;
    
    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = NRF_FICR->CODESIZE - 19;          // Use the page just below the BLE Bond Data 

    addr = (uint32_t *)(pg_size * pg_num);

    flashPageErase(addr);

    uint32_t *b = (uint32_t *) block;

    for (int i = 0; i < wordsToWrite; i++)
    {
        flashWordWrite(addr, *b);
        addr++;
        b++;
    }

    return MICROBIT_OK;
}

