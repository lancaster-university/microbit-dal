#ifndef MICROBIT_FILE_H_
#define MICROBIT_FILE_H_
#include "MicroBitFile_config.h"
#include "MicroBitFlash.h"
#include "MicroBit.h"

/**
  * @file MicroBitFile.h
  * @author Alex King
  * @date 6 Mar 2016
  *
  * @brief API for POSIX-like file system interface: open/close/read/write.
  *
  * -- Basic overview of how the file system works:
  * All file meta data is stored in the first page. Each file has a record
  * entry (struct mbr_t), which stores:
  *  - Filename
  *  - File size
  *  - An enumerated list of blocks that store the file data itself.
  * 
  * The table housing these records is referred to as the Master Block
  * Record (MBR), and looks something like this:
  *  {"file1.txt",   5000,   {1,2,3,6}}  // 5000-byte file, in blocks 1,2,3,6
  *  {"file02.txt",   200,   {5}}        // 200 byte file in a single block
  * 
  * The first MBR entry is reserved, and is used only to store a list of 
  * currently available blocks for writing.
  * Thus to expand a file size, a block must be removed from this list, and 
  * added to that files mbr_t blocklist.
  * 
  * Since there can only be one MBR, the number of entries is restricted
  * to how many can fit in a single flash page. Hence, this is the 
  * maximum number of files that can be stored.
  * 
  * -- API overview.
  * Source code is divided logically into two parts:
  * mbr_*() functions - to access and modify the mbr entries.
  *                     E.g., to create a new file, change a files size,
  *                     or append new blocks to the file.
  *  			These are private methods internal to the class.
  * 
  * read/write/seek/unlink - POSIX-style file access functions.
  *                          These are the functions used to interact with the
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
  * @todo Reduce the number of mbr_set_filesize() by caching filesize
  * information in the tinyfs_fd struct.
  * @todo implement simple wear levelling for file data writes.
  */

// mbr_t.flags field to indicate if an MBR is free/busy.
#define MBR_FREE 0x80000000

// mbr_t.flags field mask to get the file length.
#define MBR_SIZE_MASK 0x7FFFFFFF

// Used in mbr_t.blocks to indicate if a block is free.
#define MBR_FREE_BLOCK_MARKER 0x80

// Check if a given MBR is free.
#define MBR_IS_FREE(mbr) ( (mbr).flags & MBR_FREE)

// open() flags.
#define MB_READ 0x01
#define MB_WRITE 0x02
#define MB_CREAT 0x04
#define MB_FD_BUSY 0x10

// seek() flags.
#define MB_SEEK_SET 0x01
#define MB_SEEK_END 0x02
#define MB_SEEK_CUR 0x04

// Macro to read the file size from an mbr_t pointer.
#define mbr_get_filesize(m)  (m->flags & MBR_SIZE_MASK)

// return the indexed block number from the mbr_t block list.
#define mbr_get_block(m,b) (m->blocks[b])

// Obtain pointer to the indexed mbr entry.
#define mbr_by_id(index) (&mbr_loc[index])

/**
  * @brief MBR entry struct, for each file.
  * 
  * The MBR struct is the central source of metadata for each file. It stores:
  *  - Filename
  *  - File size
  *  - List of blocks that constitute the file.
  *  
  * The first mbr_t entry is reserved, and stores instead the list of currently
  * available data blocks (the free block list).
  * 
  * Modifications to these structs should be done through mbr_*() methods.
  */
typedef struct mbr_t {
   
    // Filename, must be null-terminated.
    char name[FILENAME_LEN];	
   
    // Flags/length field.
    // [31] marks the MBR as free/busy (busy=0)
    // [0-30] stores the file size.
    uint32_t flags;	
   
    // Ordered list of blocks in the file. Each uint8_t element corresponds to 
    // a data block:
    // [7] 	Only used in free block list to mark block as busy/free
    //		(1 = free, 0 = busy).
    // [0-6]	Block number.
    uint8_t blocks[(DATA_BLOCK_COUNT)]; 	
   				
} mbr;

/**
  * @brief struct for each file descriptor
  *
  * - read/write/create flags.
  * - current seek position.
  * - pointer to the files' mbr struct.
  */
typedef struct tinyfs_fd_t {
    
    // read/write/creat flags.
    uint8_t flags;
   
    // current seek position.
    int32_t seek; 
   
    // pointer to the files' mbr struct.
    // A tinyfs_fd_t struct cannot be in use without a valid mbr_entry pointer.
    mbr* mbr_entry; 
   
} tinyfs_fd;

/**
  * @brief Class definition for the MicroBit File system
  *
  * Microbit file system class. Presents a POSIX-like interface consisting of:
  * - open()
  * - close()
  * - read()
  * - write()
  * - seek()
  * - unlink()
  *
  * Only a single instance shoud exist at any given time.
  */
class MicroBitFile
{
    private:
  
    // The instance of MicroBitFlash - the interface used for all 
    // flash writes/erasures 
    MicroBitFlash flash; 
  
  
    // ** MBR-specific members **//
  
    // Pointer to the MBR struct listing unused blocks.
    mbr* mbr_free_loc = NULL; 
  
    // Pointer to the MBR table, storing the file mbr entries.
    mbr* mbr_loc = NULL;
  
    // Total number of MBR entries for file data 
    // (excludes the mbr_fre_loc entry)
    uint8_t mbr_entries = 0;
  
  
    // ** MicroBitFlash API members (above the MBR). **//
  
    // Location of the start of the flash data blocks used for file data.
    // *Excluding* the flash page reserved for MBR entries.
    uint8_t* flash_start = NULL; 
  
    // Number of flash pages for file data.
    // *Excluding* the MBR page.
    int flash_pages = 0; 
  
    // The tinyfs_fd table, for open files.
    tinyfs_fd fd_table[MAX_FD];

    // ** MBR-specific methods **/

    /**
      * @brief Find an MBR entry by name.
      *
      * @param filename name of file to search for. Must be null-terminated.
      * @return pointer to the MBR struct if found, NULL otherwise or on error.
      */
    mbr* mbr_by_name(char const * const filename);
  
    /**
      * @brief Get a pointer to the lowest numbered available MBR entry.
      *
      * @return pointer to the MBR if successful, 
      * 	NULL if there are none available.
      */
    mbr* mbr_get_free();
 
    /**
      * @brief initialize the mbr struct to a new file.
      *
      * Initialize the mbr struct with the given, null-terminated filename.
      * Ths is used to add a new file to the MBR.
      * This function also sets the file size to zero, 
      * and marks the MBR as busy.
      *
      * @param m the mbr struct to use fill.
      * @param filename the null terminated filename to use.
      * @return non-zero on success, zero on error.
      */
    int mbr_add(mbr* m, char const * const filename);
  
    /**
      * @brief add a new block to an MBR entry
      *
      * Add the numbered block to the list of blocks in an  MBR entry.
      * This function is used to expand the storage capacity for a file.
      * 
      * @param m the mbr entry/struct to add too.
      * @param blockNo the numbered block to append to the block list of a
			files' MBR.
      * @return non-zero on success, zero on error.
      */
    int mbr_add_block(mbr* m, uint8_t blockNo);

    /**
      * @brief reset an MBR entry - remove a file from the MBR table.
      *
      * Reset an MBR entry, thereby removing a file from the system.
      * This function also frees all of the blocks allocated to m,
      * back to the list of free blocks.
      *
      * @param m the mbr entry/struct to remove.
      * @return non-zero on success, zero on error.
      */
    int mbr_remove(mbr* m);
  

    /**
      * @brief obtains and marks as busy, an unused block.
      *
      * The first MBR entry is reserved, and stores the list of unused blocks.
      * Thiis function implements a stack, to pop a free block, mark as busy
      * (subsequent calls will not find the same block), and return its number.
      *
      * @return block number on success, -1 on error (out of space).
      */
    int mbr_pop_free_block();
  
    /**
      * @brief set the filesize of an mbr
      *
      * Set, in the MBR, the filesize of the entry, m.
      * 
      * @param m the MBR entry/file to modify
      * @param filesize the new filesize.
      * @return non-zero on success, zero on error.
      */
    int mbr_set_filesize(mbr* m, uint32_t filesize);

     /**
       * @brief initialize the MBR API.
       *
       * This function initializes the API for subsequent calls,
       * particularly storing the flash location, and number of entries.
       * This function must be called before any other.
       * 
       * @param mbr_location Location in flash of where to store the MBR.
       * @param mbr_entries The number of entries in the MBR
       *                    (hence determining the max no. files).
       * @return non-zero on success, zero on error.
      */
    int mbr_init(void* mbr_location, int mbr_entries);
 
    /**
      * @brief reset the MBR.
      *
      * The MBR must be in an initial state for it to work.
      * Namely, all of the mbr entries must be set to empty (0xFF).
      * and the stack of unused blocks must be populated.
      *
      * This function also sets the MAGIC_WORD as the first word in 
      * the MBR block, used to determine if the MBR has already been 
      * configured.
      *
      * This function should be called from the init() function.
      * 
      * @return non-zero on success, zero otherwise.
      */
    int mbr_build();

    /**
      * @brief Initialize the flash storage system
      *
      * This method:
      * - calls mbr_init().
      * - stores the location of the flash memory.
      * - calls mbr_build()
      *
      * @return non-zero on success, zero on error.
      */
    int init();


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
      * If the seek pointer is set beyond the current end of the file,
      * which is then written to, the file is padded with 0x00.
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
    int open(char const * const filename, uint8_t flags);
  
    /**
      * Close the specified file handle.
      * File handle resources are then made available for future open() calls.
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
           print("error writing");
      * @endcode
      */
    int write(int fd, uint8_t* buffer, int len);
  
    /**
      * Read data from the file.
      *
      * Read len bytes from the current seek position in the file, into the buffer.
      * On each invocation to read, the seek position of the file handle
      * is incremented atomically, by the number of bytes returned.
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
      * if(!f.unlink("file.txt"))
      *     print("file could not be deleted")
      * @endcode
      */
    int unlink(char const * const filename);
};

#endif
