#ifndef _TINYFS__CONFIG_
#define _TINYFS__CONFIG_ 1

// Used for TESTING, set a custom (e.g. obtained with malloc) flash start pointer, and ignore PAGE_BOUNDARY checks.
#ifndef TESTING
#define TESTING 0
#endif

// Memory address of the start of flash.
// Must be aligned on a page boundary.
#define FLASH_START 0x2F000
#ifndef FLASH_START
#warning FLASH_START not set, using default 0x3000
#define FLASH_START 0x3000
#endif

// Size of a page in flash.
#define PAGE_SIZE 1024

// Maximum number of open FD's.
// Reducing this number reduces RAM footprint.
#define MAX_FD 3

// Maximum filename length. 
// This includes the null terminator.
#define FILENAME_LEN 14

#define MAX_FILENAME_LEN FILENAME_LEN-1

// MAGIC word at the beginning of the flash region/FT table. Not really necessary to configure/change.
#define MAGIC_WORD 0xA3E8F1C7

// The number of flash pages available to the file system.
// The total number available for flash storage is 1 less: used for the FT table.
// This can be no greater than (2^7-1 = 127), as block numbers are stored in uint8_t's.
#define DATA_BLOCK_COUNT 40

// Number of FT entries in the FT table. Since this is a flat file system, without directories, this determines the maximum number of files the system can hold.
// The number of FT entries includes the free mbr list. 
// Therefore, the maximum number of files will actually be NO_FT_ENTRIES-1.
// The FT must fit in a single page.
#define NO_FT_ENTRIES 10

/** --  Pre-compiler validation checks -- **/
#if ( ((FLASH_START % PAGE_SIZE) != 0) && !TESTING )
#error FLASH_START must be on a page boundary.
#endif

#if NO_FT_ENTRIES < 2
#error NO_FT_ENTRIES must be at least 2.
#endif

#if ( (FILENAME_LEN + DATA_BLOCK_COUNT + 4 ) * NO_FT_ENTRIES ) > PAGE_SIZE
#error NO_FT_ENTRIES is too large, cannot fit in a single page.
#endif

#if DATA_BLOCK_COUNT > 127
#error DATA_BLOCK_COUNT cannot be greater than (2^7-1 = 127).
#endif

#endif
