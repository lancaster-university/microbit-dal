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
 * Class is divided into two parts: public POSIX methods:
 * open/close/read/write/unlink
 *
 * And private Master Boot Record methods, to 
 * access and change entries in the MBR page - which stores file 
 * state information.
 */

/**
 * @biief MBR entry struct, for each file.
 *
 * The MBR entry struct, for each file in the MBR.
 * This struct is not supposed to be operated on directly,
 * rather via the API in mbr.h
*/
typedef struct mbr_t {

 char name[FILENAME_LEN]; /**< file name, must be nullterminated */

 uint32_t flags; /**< Flags/length field.
                      MSB is to mark the MBR as busy/free (BUSY = 0),
                      remaining 30 bits mark the file size. */

 uint8_t blocks[(DATA_BLOCK_COUNT)]; /**< List of blocks in the file.
                                          Initially set to 0xFF
                                          Must be reset to 0xFF if removed. */
} mbr;

// Used in the mbr_t.flags field to indicate if the mbr is in use.
#define MBR_BUSY 0x00000000
#define MBR_FREE 0x80000000

// Mask with the mbr_t.flags to get the file length.
#define MBR_SIZE_MASK 0x7FFFFFFF

// Used in mbr_t.blocks to indicate if a block is free.
#define MBR_FREE_BLOCK_MARKER 0x80
#define MBR_IS_FREE(mbr) ( (mbr).flags & MBR_FREE)


/**
 * @brief struct for each file descriptor
 *
 * Not to be interacted directly by users,
 * which should only be done through the API calls.
 */
typedef struct tinyfs_fd_t {
 uint8_t flags; /**< read/write/create flags. */
 int32_t seek; /**< current seek position */
 mbr* mbr_entry; /**< pointer to the mbr struct for this file. */
} tinyfs_fd;

// open() flags.
#define MB_READ 0x01
#define MB_WRITE 0x02
#define MB_CREAT 0x04
#define MB_FD_BUSY 0x10

// seek() flags.
#define MB_SEEK_SET 0x01
#define MB_SEEK_END 0x02
#define MB_SEEK_CUR 0x04


/**
 * @brief macro to read the file size.
 *
 * @param m a pointer to the MBR struct.
 */
#define mbr_get_filesize(m)  (m->flags & MBR_SIZE_MASK)

/*
 * @brief get the enumerated value of the bth block, for MBR m
 *
 * @param m the MBR struct
 * @param b the block index.
 */
#define mbr_get_block(m,b) (m->blocks[b])

/*
 * @brief Obtain pointer to the indexed MBR struct
 *
 * @param index index of the required MBR.
 **/
#define mbr_by_id(index) (&mbr_loc[index])


class MicroBitFile
{
private:
  uint8_t* flash_start = NULL; /**< Location of the start of flash data blocks
				    *Excluding* MBR entries. */

  int flash_pages = 0; /**< Number of flash pages for data - *excluding*
			    MBR page*/

  tinyfs_fd fd_table[MAX_FD]; /**< FD table, for open files. */

  MicroBitFlash flash; /**< Instance of MicroBitFlash class, for flash_write/
			    erase/memset */

  // MBR-specific properties.

  mbr* mbr_free_loc = NULL; /**< MBR struct listing unused blocks */

  mbr* mbr_loc = NULL; /**< MBR table pointer. */

  uint8_t mbr_entries = 0; /** No. of MBR entries (excluding mbr_free_loc */

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
   * @return pointer to the MBR if successful, NULL if there are none available.
   */
  mbr* mbr_get_free();
  
  /**
   * @brief initialize the mbr struct to the set filename
   *
   * Initialize the mbr struct with the given, null-terminated filename.
   * Ths is used to add a new file to the MBR.
   * This function also sets the file size to zero, and marks the MBR as busy.
   *
   * @param m the mbr struct to use fill.
   * @param filename is the null terminated filename to use.
   * @return non-zero on success, zero on error.
   */
  int mbr_add(mbr* m, char const * const filename);
  
  /**
   * @brief remove an mbr entry from the MBR.
   *
   * Remove an MBR entry from the MBR, thereby removing a file from the system.
   * This function also frees all of the blocks allocated to m,
   * back to the list of free blocks.
   *
   * @param m the mbr entry/struct to remove.
   * @return non-zero on success, zero on error.
   */
  int mbr_remove(mbr* m);
  
  /**
   * @brief add a new block to an MBR entry
   *
   * Add the numbered block to the list of blocks assigned to an MBR entry/file.
   * This function is used to expand the storage capacity for a file.
   * @param m the mbr entry/struct to add too.
   * @param blockNo the numbered block to append to the block list for the entry.
   * @return non-zero on success, zero on error.
   */
  int mbr_add_block(mbr* m, uint8_t blockNo);

  /**
   * Initialize the flash storage system.
   *
   * Initialize the flash storage system, save the location of the flash memory.
   *
   * The MBR is always stored in the first page in flash,
   * subsequent pages are used for data storage.
   *
   * @return non-zero on success, zero on error.
   */
  int init();

  /**
   * @brief obtains and marks as busy, an unused block.
   *
   * TinyFS implements a list of unused blocks.
   * This function implements a stack, pop free block, mark as busy and return
   * its number.
   *
   * @return block number on success, -1 on error (out of space).
   */
  int mbr_pop_free_block();
  
  
  /**
   * @brief set the filesize of m
   *
   * Set, in the MBR, the filesize of the entry, m.
   * This function is used to mark a file as having extended.
   * @param m the MBR entry/file to modify
   * @param filesize the new filesize.
   * @return non-zero on success, zero on error.
   */
  int mbr_set_filesize(mbr* m, uint32_t filesize);
  
  /**
   * @brief reset the MBR.
   *
   * The MBR must be in an initial state for it to work.
   * Namely, all of the mbr entries must be set to empty,
   * and the stack of unused blocks must be populated.
   *
   * This function also sets the MAGIC_WORD as the first word in the MBR block,
   * which is used to determine if the MBR has already been configured.
   *
   * This function should be called from the init() function of TinyFS.
   * @return non-zero on success, zero otherwise.
   */
  int mbr_build();

public:
  /*
   * Constructor.
   */ 
  MicroBitFile();

  /**
   * @brief initialize the MBR API.
   *
   * This function initializes the API for subsequent calls,
   * particularly storing the flash location, and number of entries.
   * This function must be called before any other.
   * @param mbr_location Location in flash of where to store the MBR.
   * @param mbr_entries The number of entries in the MBR
   * (hence determining the max no. files).
   * @return non-zero on success, zero on error.
  */
  int mbr_init(void* mbr_location, int mbr_entries);

  /**
   * Open a new file, and obtain a new file handle to read/write/seek the file.
   * The flags are:
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
   *
   * @param filename name of the file to open, must be null terminated.
   * @param flags
   * @return return the file handle, >= 0, or < 0 on error.
   */
  int open(char const * const filename, uint8_t flags);
  
  /**
   * Close the specified file handle.
   * File handle resources are then made available for future open() calls.
   *
   * @param fd file descriptor - obtained with open().
   * @return non-zero on success, zero on error.
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
   */
  int unlink(char const * const filename);
};

#endif
