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

#ifndef MICROBIT_FLASH_H_
#define MICROBIT_FLASH_H_

#include <mbed.h>

#define PAGE_SIZE 1024

class MicroBitFlash
{
    private:

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
    int flash_write(void* address, void* buffer, int length, 
                    void* scratch_addr = NULL);

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

};

#endif
