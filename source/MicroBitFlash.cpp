#include "MicroBit.h"
#include "MicroBitFlash.h"
#include "MicroBitFile_config.h"

#define WORD_ADDR(x) (((uint32_t)x) & 0xFFFFFFFC)
#define MIN(a,b) (a<b?a:b)

/**
  * Default Constructor
  */
MicroBitFlash::MicroBitFlash() {
  this->flash_start = (uint32_t*)FLASH_START;
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
int MicroBitFlash::need_erase(uint8_t* source, uint8_t* flash_addr, int len) {
    // Erase is necessary if for any byte:
    // O & ~N != 0
    // Where O = original, and N = new byte.

    for(;len>0;len--) {
        if((~*(flash_addr++) & *(source++)) != 0x00) return 1;
    }
    return 0;
}

/**
  * Erase an entire page
  * @param page_address address of first word of page
  */
void MicroBitFlash::erase_page(uint32_t* pg_addr) {

    // Turn on flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) { }

    // Erase page:
    NRF_NVMC->ERASEPAGE = (uint32_t)pg_addr;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) { }

    // Turn off flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
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
void MicroBitFlash::flash_burn(uint32_t* addr, uint32_t* buffer, int size) {
 
    // Turn on flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {};
 
    for(int i=0;i<size;i++) {
        *(addr+i) = *(buffer+i);
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {};
    }
 
    // Turn off flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {};
}
 
/**
  * Write to address in flash, implementing either flash_write (copy
  * data from buffer), or flash_memset (set all bytes in flash to
  * provided constant.
  *
  * Function ensures that data is written correctly:
  * - erase page if necessary (using need_erase)
  * - preserve non-target flash by copying to scratch page.
  *
  * @param address in flash to write to
  * @param from_buffer address to write data from
  * @param write_byte if writing the same byte to add addressed bytes
  * @param len number of bytes to write to/copy to
  * @param m mode to use this function, memset/memcpy.
  * @return non-zero on success, zero on error.
  */
int MicroBitFlash::flash_write_mem(uint8_t* address, uint8_t* from_buffer,
    uint8_t write_byte, int length, flash_mode m) {

    // page number.
    int page = (address - (uint8_t*)flash_start) / PAGE_SIZE;

    //page address.
    uint32_t* pgAddr = flash_start + (page * (PAGE_SIZE/sizeof(uint32_t)));

    // offset to write from within page.
    int offset = (address - (uint8_t*)flash_start) % (int)PAGE_SIZE;

    // uBit.serial.printf("flash_write to 0x%x, from 0x%x, length: 0x%x\n",
    //       	     address, from_buffer, length);
    // uBit.serial.printf(" - offset = %d, pgAddr = 0x%x, page = %d\n",
    //                    offset, pgAddr, page);

    uint8_t* writeFrom = (uint8_t*)pgAddr;
    int start = WORD_ADDR(offset);
    int end = WORD_ADDR((offset+length+4));

    if(need_erase(from_buffer, address, length)) {
        this->erase_page((uint32_t*)SCRATCH_PAGE_ADDR);
        this->flash_burn((uint32_t*)SCRATCH_PAGE_ADDR, pgAddr, PAGE_SIZE/4);
        this->erase_page(pgAddr);
        writeFrom = (uint8_t*)SCRATCH_PAGE_ADDR;
        start = 0;
        end = PAGE_SIZE;
    }

    uint32_t writeWord = 0;

    for(int i=start;i<end;i++) {
        int byteOffset = i%4;

        if(i >= offset && i < (offset + length)) {
            if(m == WR_WRITE) {
                // Write from buffer.
                writeWord |= (from_buffer[i-offset] << ((byteOffset)*8));
            }
            else if(m == WR_MEMSET) {
                // Write constant.
                writeWord |= ((uint32_t)write_byte << (byteOffset*8));
            }
        }
        else {
            writeWord |= (writeFrom[i] << ((byteOffset)*8));
        }

        if( ((i+1)%4) == 0) {
            this->flash_burn(pgAddr + (i/4), &writeWord, 1);
            writeWord = 0;
        }
    }

    return 1;
}

/**
  * Writes the given number of bytes to the address in flash specified.
  * Neither address nor buffer need be word-aligned.
  * @param address location in flash to write to.
  * @param buffer location in memory to write from.
  * @length number of bytes to burn
  * @return non-zero on sucess, zero on error.
  *
  * Example:
  * @code
  * MicroBitFlash flash();
  * uint32_t word = 0x01;
  * flash.flash_write((uint8_t*)0x38000, &word, sizeof(word))
  * @endcode
  */
int MicroBitFlash::flash_write(uint8_t* address, uint8_t* from_buffer, 
                               int length) {
    return this->flash_write_mem(address, from_buffer, 0, length, WR_WRITE);
}

/**
  * Set bytes in [address, address+length] to byte.
  * Neither address nor buffer need be word-aligned.
  * @param address to write to
  * @param byte byte to burn to flash
  * @param length number of bytes to write to.
  * @return non-zero on success, zero on error.
  *
  * Example:
  * @code
  * MicroBitFlash flash();
  * flash.flash_memset((uint8_t*)0x38000, 0x66, 12); //12 bytes to 0x66.
  * @endcode
  */
int MicroBitFlash::flash_memset(uint8_t* address, uint8_t write_byte, 
                                int length) {
    return this->flash_write_mem(address, NULL, write_byte, length, WR_MEMSET);
}

/**
  * Erase bytes in memory, from set address.
  * @param address to erase bytes from (needn't be word-aligned.
  * @param length number of bytes to erase (set to 0xFF).
  * @return non-zero on success, zero on error.
  *
  * Example:
  * @code
  * MicroBitFlash flash();
  * flash.flash_erase((uint8_t*)0x38000, 10240); //erase a flash page.
  * @endcode
  */
int MicroBitFlash::flash_erase_mem(uint8_t* address, int length) {
    return this->flash_write_mem(address, NULL, 0xFF, length, WR_MEMSET);
}

