#ifndef MICROBIT_FILE_H_
#define MICROBIT_FILE_H_
#include "MicroBitFlash.h"
#include <stdint.h>

/**
  * @file MicroBitFile.h
  * @author Alex King
  * @date 6 Mar 2016
  *
  * @brief API for POSIX-like file system interface: open/close/read/write.
  *
  * -- Basic overview of how the file system works:
  * All file meta data is stored in the first page. Each file has a record
  * entry (struct FileTableEntry_t), which stores:
  *  - Filename
  *  - File size
  *  - An ordered list of pages/blocks that store the file data itself.
  * 
  * The table housing these records is referred to as the File Table (FT)
  * and looks something like this:
  *  {"file1.txt",   5000,   {1,2,3,6}}  // 5000-byte file, in blocks 1,2,3,6
  *  {"file02.txt",   200,   {5}}        // 200 byte file in a single block
  * 
  * The first FT entry is reserved, and is used only to store a list of 
  * currently available blocks for writing.
  * Thus to expand a file size, a block must be removed from this list, and 
  * added to that files blocklist.
  * 
  * Since there can only be one FT, the number of entries is restricted
  * to how many can fit in a single flash page. Hence, this is the 
  * maximum number of files that can be stored.
  * 
  * The filesize is stored in the FT, but cached in the mb_fd struct.
  * Writes to the FT filesize are made sparingly in write() and close(), to
  * reduce erasures.
  * @warning close() *must* be called to ensure the FT is synched, before 
  * the microbit is finished.
  * 
  * -- API overview.
  * Source code is divided logically into two parts:
  * ft_*() functions - to access and modify the FT entries.
  *                    E.g., to create a new file, change a files size,
  *                    or append new blocks to the file.
  *                    These are private methods internal to the class.
  * 
  * read/write/seek/remove - POSIX-style file access functions.
  *                          These are the methods used to interact with the
  *                          file system.
  * 
  * Example:
  * @code
  * MicroBitFile f;
  * int fd = f.open("myFile.txt", MB_WRITE|MB_CREAT);
  * if(fd < 0) {
  *   print("couldn't open a new file");
  *   exit;
  * }
  * if(f.write(fd, "Hello, World!\n",14) != 14) {
  *   print("error writing!")
  *   exit
  * }
  * f.close(fd);
  * @endcode
  * 
  * -- Notes
  * - There should exist only one instance of this class.
  * @todo implement simple wear levelling for file data writes.
  * @todo implement MB_APPEND flag in open()
  */

// Configuration options.

#define PAGE_SIZE 1024  // Size of page in flash.

#define MAX_FD 4  // Max number of open FD's.

#define FILENAME_LEN 16 // Filename length, including \0

#define MBFS_LAST_PAGE_ADDR 0x3AC00 // Address of last writable page in flash.

#define MAX_FILESYSTEM_PAGES 40 // Max. number of pages available.

#define NO_FT_ENTRIES  10  // Number of entries in the File table  
                            // (determines total no. files on the system)

#if ( (FILENAME_LEN + MAX_FILESYSTEM_PAGES + 4 ) * NO_FT_ENTRIES ) > PAGE_SIZE
#error NO_FT_ENTRIES is too large, file table must fit in a single page.
#endif

#if MAX_FILESYSTEM_PAGES > 127
#error MAX_FILESYSTEM_PAGES cannot be greater than 127.
#endif 

#define MAX_FILENAME_LEN FILENAME_LEN-1

#define MAGIC_WORD 0xA3E8F1C7


// FileTableEntry_t.flags field to indicate if an FT is free/busy.
#define FT_FREE 0x80000000
#define FT_BUSY 0x00000000

// FileTableEntry_t.flags field mask to get the file length.
#define FT_SIZE_MASK 0x7FFFFFFF

// Used in FileTableEntry_t.blocks to indicate if a block is free.
#define FT_FREE_BLOCK_MARKER 0x80

// Check if a given FileTableEntry is free.
#define FT_IS_FREE(m) ( (m).flags & FT_FREE)

// Macro to read the file size from an FileTableEntry_t pointer.
#define ft_get_filesize(m)  (m->flags & FT_SIZE_MASK)

// return the indexed block number from the FileTableEntry_t block list.
#define ft_get_block(m,b) (m->blocks[b])

// Obtain pointer to the indexed FileTableEntry entry.
#define ft_by_id(index) (&ft_loc[index])

// Check if a FileTableEntry pointer is within table.
#define FT_PTR_VALID(p) ( (p >= this->ft_loc) && (p-this->ft_loc)<=(this->ft_entries-1) )

// open() flags.
#define MB_READ 0x01
#define MB_WRITE 0x02
#define MB_CREAT 0x04
#define MB_APPEND 0x08

// seek() flags.
#define MB_SEEK_SET 0x01
#define MB_SEEK_END 0x02
#define MB_SEEK_CUR 0x04

// Test if init()/ft_init have been called.
#define FS_INITIALIZED() (this->flash_start != NULL)
#define FT_INITIALIZED() (this->ft_loc != NULL)

/**
  * @brief FT entry struct, for each file.
  * 
  * The FT struct is the central source of metadata for each file. It stores:
  *  - Filename
  *  - File size
  *  - List of blocks that constitute the file.
  *  
  * The first FileTableEntry_t entry is reserved, and stores instead 
  * the list of currently available data blocks (the free block list).
  * 
  * Modifications to these structs should be done through ft_*() methods.
  */
typedef struct FileTableEntry_t 
{
   
    // Filename, must be null-terminated.
    char name[FILENAME_LEN];	
   
    // Flags/length field.
    // [31] marks the FT entry as free/busy (busy=0)
    // [0-30] stores the file size.
    uint32_t flags;	
   
    // Ordered list of blocks in the file. Each uint8_t element corresponds to 
    // a data block:
    // [7] 	    Only used in free block list to mark block as busy/free
    //		    (1 = free, 0 = busy).
    // [0-6]	Block number.
    uint8_t blocks[(MAX_FILESYSTEM_PAGES)]; 	
   				
} FileTableEntry;

/**
  * @brief struct for each file descriptor
  *
  * - read/write/create flags.
  * - current seek position.
  * - pointer to the files' FileTableEntry struct.
  * The file size is stored in the FT, but cached here.
  * Writes to the FT are made sparingly in mb_write() and mb_close()
  * to reduce erasures.
  */
typedef struct mb_fd_t 
{
    
    // read/write/creat flags.
    uint8_t flags;
   
    // current seek position.
    int32_t seek; 
  
    // the cached file size.
    int32_t filesize;
   
    // pointer to the files' FileTableEntry struct.
    // A mb_fd_t struct cannot be in use without a FileTabeEntry 
    // ft_entry pointer.
    FileTableEntry* ft_entry; 
   
} mb_fd;

/**
  * @brief Class definition for the MicroBit File system
  *
  * Microbit file system class. Presents a POSIX-like interface consisting of:
  * - open()
  * - close()
  * - read()
  * - write()
  * - seek()
  * - remove()
  *
  * Only a single instance shoud exist at any given time.
  */
class MicroBitFile
{
    private:
  
    // The instance of MicroBitFlash - the interface used for all 
    // flash writes/erasures 
    MicroBitFlash flash; 
  
  
    // ** FT-specific members **//
  
    // Pointer to the FT struct listing unused blocks.
    FileTableEntry* ft_free_loc = NULL; 
  
    // Pointer to the FT table, storing the file FileTableEntry entries.
    FileTableEntry* ft_loc = NULL;
  
    // Number of pages available for file data (excludes file table page)
    int flash_data_pages;
  
  
    // ** MicroBitFlash API members (above the FT). **//
  
    // Location of the start of the flash data blocks used for file data.
    // *Excluding* the flash page reserved for FT entries.
    uint8_t* flash_start = NULL; 
  
    // The mb_fd pointer table, for open files.
    mb_fd * fd_table[MAX_FD];

    // ** FT-specific methods **/

    /**
      * @brief Find an FT entry by name.
      *
      * @param filename name of file to search for. Must be null-terminated.
      * @return pointer to the FT struct if found, NULL otherwise or on error.
      */
    FileTableEntry* ft_by_name(char const * filename);
  
    /**
      * @brief Get a pointer to the lowest numbered available FT entry.
      *
      * @return pointer to the FT if successful, 
      * 	NULL if there are none available.
      */
    FileTableEntry* ft_get_free();
 
    /**
      * @brief initialize the FileTableEntry struct to a new file.
      *
      * Initialize the FileTableEntry struct with the null-terminated filename.
      * Ths is used to add a new file to the FT.
      * This function also sets the file size to zero, 
      * and marks the FT as busy.
      *
      * @param m the FileTableEntry struct to use fill.
      * @param filename the null terminated filename to use.
      * @return non-zero on success, zero on error.
      */
    int ft_add(FileTableEntry* m, char const * filename);
  
    /**
      * @brief add a new block to an FT entry
      *
      * Add the numbered block to the list of blocks in an  FT entry.
      * This function is used to expand the storage capacity for a file.
      * 
      * @param m the FileTableEntry entry/struct to add too.
      * @param blockNo the numbered block to append to the block list of a
      *        files' FT.
      * @return non-zero on success, zero on error.
      */
    int ft_add_block(FileTableEntry* m, uint8_t blockNo);

    /**
      * @brief reset an FT entry - remove a file from the FT table.
      *
      * Reset an FT entry, thereby removing a file from the system.
      * This function also frees all of the blocks allocated to m,
      * back to the list of free blocks.
      *
      * @param m the FileTableEntry entry/struct to remove.
      * @return non-zero on success, zero on error.
      */
    int ft_remove(FileTableEntry* m);
  
    /**
      * @brief obtains and marks as busy, an unused block.
      *
      * The first FT entry is reserved, and stores the list of unused blocks.
      * This function implements a stack, to pop a free block, mark as busy
      * (subsequent calls will not find the same block), and return its number.
      *
      * @return block number on success, -1 on error (out of space).
      */
    int ft_pop_free_block();
  
    /**
      * @brief set the filesize of an FileTableEntry
      *
      * Set, in the FT, the filesize of the entry, m.
      * 
      * @param m the FT entry/file to modify
      * @param filesize the new filesize.
      * @return non-zero on success, zero on error.
      */
    int ft_set_filesize(FileTableEntry* m, uint32_t filesize);

     /**
       * @brief initialize the FT API.
       *
       * This function initializes the API for subsequent calls,
       * particularly storing the flash location, and number data pages.
       * This function must be called before any other (including ft_build).
       * 
       * @param ft_location Location in flash of where to store the FT.
       * @param flash_data_blocks the number of data blocks/pages reserved
       *        for file data.
       */
    void ft_init(void* ft_location, int flash_data_blocks);
 
    /**
      * @brief reset the FT.
      *
      * The FT must be in an initial state for it to work.
      * Namely, all of the FileTableEntry entries must be set to empty (0xFF).
      * and the stack of unused blocks must be populated.
      *
      * This function also sets the MAGIC_WORD as the first word in 
      * the FT block, used to determine if the FT has already been 
      * configured.
      *
      * This function should be called from the init() function.
      * 
      * @return non-zero on success, zero otherwise.
      */
    int ft_build();

    /**
      * @brief Initialize the flash storage system
      * 
      * The file system is located dynamically, based on where the program code
      * and code data finishes. This avoids having to allocate a fixed flash 
      * region for builds even without MicroBitFile. The location is saved in 
      * the key value store, so that this can be easily determined by the DAPLink.
      * 
      * The file system size expands to fill the remaining space, but is capped 
      * at MAX_FILESYSTEM_PAGES.
      * 
      * This method:
      *  - calls ft_init()
      *  - checks if the file system already exists, if not calls ft_build.
      *  - saves location to MicroBitStorage key value store.
      * 
      * @return non-zero on success, zero on error.
      */
    int init();

    /**
      * @brief Pick a random block from the ft_free_loc free blocks list.
      * 
      * To be used in calls to flash_write/memset/erase, to specify a random 
      * scratch page.
      *
      * @return NULL on error, scratch page address on success
      */
    uint8_t* getRandomScratch();


    public:

    /**
      * Constructor. Calls the necessary init() functions.
      */ 
    MicroBitFile();

    /**
      * Open a new file, and obtain a new file handle (int) to 
      * read/write/seek the file. The flags are:
      *  - MB_READ : read from the file.
      *  - MB_WRITE : write to the file.
      *  - MB_CREAT : create a new file, if it doesn't already exist.
      *
      * If a file is opened that doesn't exist, and MB_CREAT isn't passed,
      * an error is returned, otherwise the file is created.
      *
      * @todo The same file can only be opened by a single handle at once.
      * @todo Add MB_APPEND flag.
      *
      * @param filename name of the file to open, must be null terminated.
      * @param flags
      * @return return the file handle, >= 0, or < 0 on error.
      * 
      * Example:
      * @code
      * MicroBitFile f();
      * int fd = f.open("test.txt", MB_WRITE|MB_CREAT);
      * if(fd<0) 
      *    print("file open error");
      * @endcode
      */
    int open(char const * filename, uint8_t flags);
  
    /**
      * Close the specified file handle.
      * File handle resources are then made available for future open() calls.
      * 
      * close() must be called at some point to ensure the filesize in the
      * FT is synced with the cached value in the FD.
      *
      * @warning if close() is not called, the FT may not be correct,
      * leading to data loss.
      *
      * @param fd file descriptor - obtained with open().
      * @return non-zero on success, zero on error.
      *
      * Example:
      * @code
      * MicroBitFile f();
      * int fd = f.open("test.txt", MB_READ);
      * if(!f.close(fd))
      *    print("error closing file.");
      * @endcode
      */
    int close(int fd);
  
    /**
      * Move the current position of a file handle, to be used for
      * subsequent read/write calls.
      *
      * The offset modifier can be:
      *  - MB_SEEK_SET set the absolute seek position.
      *  - MB_SEEK_CUR set the seek position based on the current offset.
      *  - MB_SEEK_END set the seek position from the end of the file.
      * E.g. to seek to 2nd-to-last byte, use offset=-1.
      *
      * @param fd file handle, obtained with open()
      * @param offset new offset, can be positive/negative.
      * @param flags
      * @return new offset position on success, < 0 on error.
      * 
      * Example:
      * @code
      * MicroBitFile f;
      * int fd = f.open("test.txt", MB_READ);
      * f.seek(fd, -100, MB_SEEK_END); //100 bytes before end of file.
      * @endcode
      */
    int seek(int fd, int offset, uint8_t flags);
  
    /**
      * Write data to the file.
      *
      * Write from buffer, len bytes to the current seek position.
      * On each invocation to write, the seek position of the file handle
      * is incremented atomically, by the number of bytes returned.
      *
      * The cached filesize in the FD is updated on this call. Also, the
      * FT file size is updated only if a new page(s) has been written too,
      * to reduce the number of FT writes.
      *
      * @param fd File handle
      * @param buffer the buffer from which to write data
      * @param len number of bytes to write
      * @return number of bytes written on success, < 0 on error.
      *
      * Example:
      * @code
      * MicroBitFile f();
      * int fd = f.open("test.txt", MB_WRITE);
      * if(f.write(fd, "hello!", 7) != 7)
      *    print("error writing");
      * @endcode
      */
    int write(int fd, uint8_t* buffer, int len);
  
    /**
      * Read data from the file.
      *
      * Read len bytes from the current seek position in the file, into the 
      * buffer. On each invocation to read, the seek position of the file
      * handle is incremented atomically, by the number of bytes returned.
      *
      * @param fd File handle, obtained with open()
      * @param buffer to store data
      * @param len number of bytes to read
      * @return number of bytes read on success, < 0 on error.
      *
      * Example:
      * @code
      * MicroBitFile f;
      * int fd = f.open("read.txt", MB_READ);
      * if(f.read(fd, buffer, 100) != 100) 
      *    print("read error");
      * @endcode
      */
    int read(int fd, uint8_t* buffer, int len);
  
    /**
      * Remove a file from the system, and free allocated assets
      * (including assigned blocks which are returned for use by other files).
      *
      * @todo the file must not already have an open file handle.
      *
      * @param filename null-terminated name of the file to remove.
      * @return non-zero on success, zero on error
      *
      * Example:
      * @code
      * MicroBitFile f;
      * if(!f.remove("file.txt"))
      *     print("file could not be removed")
      * @endcode
      */
    int remove(char const * filename);
};

#endif
