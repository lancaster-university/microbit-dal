#ifndef MICROBIT_FLASH_H_
#define MICROBIT_FLASH_H_
#include "MicroBit.h"

#define SCRATCH_PAGE_ADDR 0x2EC00
#define PAGE_SIZE 1024

typedef enum flash_mode_t {
    WR_WRITE,
    WR_MEMSET
} flash_mode;

class MicroBitFlash
{
    private:
    // Address of the first word in flash memory we can write to.
    uint32_t* flash_start; 

    /**
      * Erase an entire page.
      * @param page_address address of first word of page.
      */
    void erase_page(uint32_t* page_address);

    /**
      * Write to flash memory, assuming that a write is valid
      * (using need_erase).
      * 
      * @param page_address address of memory to write to. 
      * 	Must be word aligned.
      * @param buffer address to write from, must be word-aligned.
      * @param len number of uint32_t words to write.
      */
    void flash_burn(uint32_t* page_address, uint32_t* buffer, int len);

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
    int flash_write_mem(uint8_t* address, uint8_t* from_buffer,
                        uint8_t write_byte, int len, flash_mode m);

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
    int need_erase(uint8_t* source, uint8_t* flash_addr, int len);
 
    public:
    /**
      * Default constructor.
      */
    MicroBitFlash();

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
    int flash_write(uint8_t* address, uint8_t* buffer, int length);

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
    int flash_memset(uint8_t* address, uint8_t byte, int length);

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
    int flash_erase_mem(uint8_t* address, int length);

};

#endif
