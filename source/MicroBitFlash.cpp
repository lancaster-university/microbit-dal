#include "MicroBit.h"
#include "MicroBitFlash.h"
#include "tinyfs_config.h"

#define WORD_ADDR(x) (((uint32_t)x) & 0xFFFFFFFC)
#define MIN(a,b) (a<b?a:b)

MicroBitFlash::MicroBitFlash() {
  this->flash_start = (uint32_t*)FLASH_START;
}

// Erase is necessary if for any byte:
// O & ~N != 0
// Where O = original, and N = new byte.
int MicroBitFlash::need_erase(uint8_t* source, uint8_t* flash_addr, int len) {
  for(;len>0;len--) {
    if((~*(flash_addr++) & *(source++)) != 0x00) return 1;
  }
  return 0;
}

// Erase an entire page. pg_addr must be first word of page.
void MicroBitFlash::erase_page(uint32_t* pg_addr) {
  //uBit.serial.printf("erase_page %d 0x%x\n", ((uint32_t)pg_addr / 256), pg_addr);

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
 
// Assumes that this is valid under flash writing rules (see erase_page).
void MicroBitFlash::flash_burn(uint32_t* addr, uint32_t* buffer, int size) {
  //uBit.serial.printf("flash_burn page %d 0x%x, from 0x%x, size %d\n",
  //     ((uint32_t)addr / 256), addr, buffer, size);
 
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
 
// Generic function for burning unaligned data, to unaligned flash addresses.
// Can come from a buffer, or a preset byte value, determined by flash_mode m
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
      if(m == WR_WRITE) writeWord |= (from_buffer[i-offset] << ((byteOffset)*8));
      else if(m == WR_MEMSET) writeWord |= ((uint32_t)write_byte << (byteOffset*8));
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

int MicroBitFlash::flash_write(uint8_t* address, uint8_t* from_buffer, int length) {
 return this->flash_write_mem(address, from_buffer, 0, length, WR_WRITE);
}
int MicroBitFlash::flash_memset(uint8_t* address, uint8_t write_byte, int length) {
 return this->flash_write_mem(address, NULL, write_byte, length, WR_MEMSET);
}
int MicroBitFlash::flash_erase_mem(uint8_t* address, int length) {
 return this->flash_write_mem(address, NULL, 0xFF, length, WR_MEMSET);
}

