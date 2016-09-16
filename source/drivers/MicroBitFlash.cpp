#include "MicroBitConfig.h"
#include "MicroBitFlash.h"
#include "ErrorNo.h"
#include "mbed.h"                   // NVIC

#define MIN(a,b) ((a)<(b)?(a):(b))

#define WORD_ADDR(x) (((uint32_t)x) & 0xFFFFFFFC)

/**
  * Default Constructor
  */
MicroBitFlash::MicroBitFlash() 
{
}

/**
  * Check if an erase is required to write to a region in flash memory.
  * This is determined if, for any byte:
  * ~O & N != 0x00
  * Where O=Original byte, N = New byte.
  *
  * @param source to write from
  * @param flash_address to write to
  * @param len number of uint8_t to check.
  * @return non-zero if erase required, zero otherwise.
  */
int MicroBitFlash::need_erase(uint8_t* source, uint8_t* flash_addr, int len) 
{
    // Erase is necessary if for any byte:
    // O & ~N != 0
    // Where O = original, and N = new byte.

    for(;len>0;len--) 
    {
        if((~*(flash_addr++) & *(source++)) != 0x00) return 1;
    }
    return 0;
}

/**
  * Erase an entire page
  * @param page_address address of first word of page
  */
void MicroBitFlash::erase_page(uint32_t* pg_addr) 
{
    // Turn on flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) { }

    // Erase page:
    NRF_NVMC->ERASEPAGE = (uint32_t)pg_addr;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) { }

    // Turn off flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) { }
}
 
/**
  * Write to flash memory, assuming that a write is valid
  * (using need_erase).
  *
  * @param page_address address of memory to write to.
  *     Must be word aligned.
  * @param buffer address to write from, must be word-aligned.
  * @param len number of uint32_t words to write.
  */
void MicroBitFlash::flash_burn(uint32_t* addr, uint32_t* buffer, int size) 
{
 
    // Turn on flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {};
 
    for(int i=0;i<size;i++) 
    {
        *(addr+i) = *(buffer+i);
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {};
    }
 
    // Turn off flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {};
}
 
/**
  * Writes the given number of bytes to the address in flash specified.
  * Neither address nor buffer need be word-aligned.
  * @param address location in flash to write to.
  * @param buffer location in memory to write from.
  * @length number of bytes to burn
  * @param scratch_addr if specified, scratch page to use. Use default
  *                     otherwise.
  * @return non-zero on sucess, zero on error.
  *
  * Example:
  * @code
  * MicroBitFlash flash();
  * uint32_t word = 0x01;
  * flash.flash_write((uint8_t*)0x38000, &word, sizeof(word))
  * @endcode
  */
int MicroBitFlash::flash_write(void* address, void* from_buffer, 
                               int length, void* scratch_addr)
{

    // Ensure that scratch_addr is aligned on a page boundary.
    if((uint32_t)scratch_addr & 0x3FF) 
        return MICROBIT_INVALID_PARAMETER;

    // Locate the hardware FLASH page used by this operation.
    int page = (uint32_t)address / PAGE_SIZE;
    uint32_t* pgAddr = (uint32_t*)(page * PAGE_SIZE);

    // offset to write from within page.
    int offset = (uint32_t)address % PAGE_SIZE;

    uint8_t* writeFrom = (uint8_t*)pgAddr;
    int start = WORD_ADDR(offset);
    int end = WORD_ADDR((offset+length+4));
    int erase = need_erase((uint8_t *)from_buffer, (uint8_t *)address, length);

    // Preserve the data by writing to the scratch page.
    if(erase) 
    {
        if (!scratch_addr)
            return MICROBIT_INVALID_PARAMETER;

        this->flash_burn((uint32_t*)scratch_addr, pgAddr, PAGE_SIZE/4);
        this->erase_page(pgAddr);
        writeFrom = (uint8_t*)scratch_addr;
        start = 0;
        end = PAGE_SIZE;
    }

    uint32_t writeWord = 0;

    for(int i=start;i<end;i++) 
    {
        int byteOffset = i%4;

        if(i >= offset && i < (offset + length)) 
        {
            // Write from buffer.
            writeWord |= (((uint8_t *)from_buffer)[i-offset] << ((byteOffset)*8));
        }
        else 
        {
            writeWord |= (writeFrom[i] << ((byteOffset)*8));
        }

        if( ((i+1)%4) == 0) 
        {
            this->flash_burn(pgAddr + (i/4), &writeWord, 1);
            writeWord = 0;
        }
    }

    return MICROBIT_OK;
}

