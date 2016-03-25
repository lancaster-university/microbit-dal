#include <stddef.h>
#include <string.h>
#include "MicroBitFile_config.h"
#include "MicroBitFile.h"
#include "MicroBitFlash.h"

#define MIN(a,b) ((a)<(b)?(a):(b))

#define DEBUG 0
#if DEBUG
#define PRINTF(...) uBit.serial.printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/**
  * @brief Find an MBR entry by name.
  *
  * @param filename name of file to search for. Must be null-terminated.
  * @return pointer to the MBR struct if found, NULL otherwise or on error.
  */
mbr* MicroBitFile::mbr_by_name(char const * const filename) {
   
    for(int i=0;i<this->mbr_entries;++i) {
        if(strcmp(filename, this->mbr_loc[i].name) == 0) {
            return &this->mbr_loc[i];
        }
    }
    return NULL;
}

/**
  * @brief Get a pointer to the lowest numbered available MBR entry.
  *
  * @return pointer to the MBR if successful,
  *         NULL if there are none available.
  */
mbr* MicroBitFile::mbr_get_free() {
   
    for(int i=0;i<this->mbr_entries;++i) {
        if(MBR_IS_FREE(this->mbr_loc[i])) return &this->mbr_loc[i];
    }
    return NULL;
}

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
int MicroBitFile::mbr_add(mbr* m, char const * const filename) {
   
    uint32_t t = MBR_BUSY;
    
    // Write the Filename.
    if(!this->flash.flash_write((uint8_t*)m->name,
                                (uint8_t*)filename, strlen(filename)+1)) {
        return 0;
    }

    // Write the flags/file size.
    if(!this->flash.flash_write((uint8_t*)&(m->flags),
                                (uint8_t*)&t, sizeof(t))) {
        return 0;
    }
         
    PRINTF("Set mbr entry, filename: %s\n", filename);
    return 1;
}

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
int MicroBitFile::mbr_add_block(mbr* m, uint8_t blockNo) {
   
    // Find the lowest unused block entry.
    int b = 0;
    for(b=0;b<DATA_BLOCK_COUNT;b++) {
 
        // The mbr_t.blocks list is terminated by 0xFF, this is the first 
        // unallocated block.
        if(m->blocks[b] == 0xFF) break;

    }
    if(b==DATA_BLOCK_COUNT) return 0;
   
    return this->flash.flash_write(&(m->blocks[b]), &blockNo, 1);
}


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
int MicroBitFile::mbr_remove(mbr* m) {

    // ----------------------------------------------
    // Return block numbers in the mbr entry to the mbr_free_loc->blocks list.
    // Add to the beginning of the list.
    //
    // So if we have before:
    // [ ][ ][ ][4][5][6]
    //
    // Inserting '1' Will then become:
    // [ ][ ][1][4][5][6]
    // ----------------------------------------------
  
    // used = no. used blocks in mbr. 
    int used=0;    
    for(used=0;used<DATA_BLOCK_COUNT;used++) {
        if( m->blocks[used] == 0xFF) break;
    }
   
    // Where to insert into free list.
    int b = 0;     
    for(b=0;b<DATA_BLOCK_COUNT;++b) {
        if(this->mbr_free_loc->blocks[b] != 0x00) break;
    }
   
    //not enough room. (this shouldn't happen)
    if(used > b) { 
        return 0;
    }
    int p = b-used;
   
    PRINTF("mbr removed. Name: %s, blocks: %d\n", m->filename, used);
  
    // Reset the free block list entries to 0xFF.
    if(!this->flash.flash_erase_mem((uint8_t*)&this->mbr_free_loc->blocks[p],
        used)) {
        return 0;
    }

    // Write each of the block numbers.   
    for(int j=0;j<used;j++) {
        uint8_t bl = m->blocks[j] | MBR_FREE_BLOCK_MARKER;

        if(!this->flash.flash_write(&this->mbr_free_loc->blocks[p+j],&bl,1)) {
            return 0;
        }
    }

    // Erase the MBR (can then be resused.
    if(!this->flash.flash_erase_mem((uint8_t*)m, sizeof(mbr))) {
        return 0;
    }

    return 1;
}

/**
  * @brief obtains and marks as busy, an unused block.
  *
  * The first MBR entry is reserved, and stores the list of unused blocks.
  * Thiis function implements a stack, to pop a free block, mark as busy
  * (subsequent calls will not find the same block), and return its number.
  *
  * @return block number on success, -1 on error (out of space).
  */
int MicroBitFile::mbr_pop_free_block() {
  
    // ----------------------------------------------
    // Find a free block in mbr_free_loc->blocks.
    // Pop from the front of the list.
    //
    // So if we have beforehand:
    // [1][2][3][4][5][6]
    //
    // Will become:
    // [ ][2][3][4][5][6]
    // ----------------------------------------------
  
    //find the lowest free block.
    int b = 0;
    for(b=0;b<DATA_BLOCK_COUNT;b++) {
         if(this->mbr_free_loc->blocks[b] != 0x00 &&
            this->mbr_free_loc->blocks[b] != 0xFF) break;
    }
    if(b==DATA_BLOCK_COUNT) {
        PRINTF("No free blocks available\n");
        return -1;
    }
  
    //mark as unavailable in the free block list.
    uint8_t write_empty = 0x00;
    uint8_t blockNumber = mbr_get_block(this->mbr_free_loc, b);
    blockNumber &= ~(MBR_FREE_BLOCK_MARKER);

    if(!this->flash.flash_write(&this->mbr_free_loc->blocks[b], 
                                &write_empty, 1)) return -1;
  
    PRINTF("Free block %d from location in list %d\n", blockNumber, b);
    return blockNumber;
}

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
int MicroBitFile::mbr_init(void* mbr_location, int mbr_no) {

    PRINTF("initializing mbr\n");

    this->mbr_free_loc = (mbr*)mbr_location;
    this->mbr_loc = (mbr*)mbr_location + 1;
    this->mbr_entries = mbr_no-1;

    return 1;
}

/**
  * @brief set the filesize of an mbr
  *
  * Set, in the MBR, the filesize of the entry, m.
  *
  * @param m the MBR entry/file to modify
  * @param filesize the new filesize.
  * @return non-zero on success, zero on error.
  */
int MicroBitFile::mbr_set_filesize(mbr* m, uint32_t fz) {
  
    PRINTF("set filesize: %s to %d bytes\n", m->name, fz);
    return this->flash.flash_write((uint8_t*)&m->flags, (uint8_t*)&fz, 4);
}

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
int MicroBitFile::mbr_build() {
  
    // Only build the MBR table if not already initialized.
    if(*((uint32_t*)this->mbr_free_loc) == MAGIC_WORD) {
        PRINTF("Magic word detected, file system already built\n");
        return 1;
    }
    PRINTF("Magic word not detected, building file system\n");
  
    uint32_t magic = MAGIC_WORD;
  
    // Erase MBR Page.
    if(!this->flash.flash_erase_mem((uint8_t*)this->mbr_free_loc, PAGE_SIZE)) {
        return 0;
    }
  
    // Write Magic.
    if(!this->flash.flash_write((uint8_t*)this->mbr_free_loc->name,
                                (uint8_t*)&magic, sizeof(magic))) {
        return 0;
    }
  
    // Populate the free block list.
    uint8_t bl[DATA_BLOCK_COUNT-1];
    for(int i=0;i<(DATA_BLOCK_COUNT-1);++i) {
        bl[i] = i | MBR_FREE_BLOCK_MARKER;
    }
  
    return this->flash.flash_write((uint8_t*)&this->mbr_free_loc->blocks, 
                                    bl, DATA_BLOCK_COUNT-1);
}

/**
  * Constructor. Calls the necessary init() functions.
  */
MicroBitFile::MicroBitFile() {
    int initialized = this->init();
    PRINTF("initializing.. %d\n", initialized);
    memset(this->fd_table, 0x00, sizeof(tinyfs_fd) * MAX_FD);
}

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
int MicroBitFile::init() {
    if(FS_INITIALIZED()) return 0;

    // MBR-specific init/build.   
    if(!this->mbr_init((uint8_t*)FLASH_START, NO_MBR_ENTRIES)) return 0;
    if(!this->mbr_build()) return 0;
   
    PRINTF("initialized\n");
     
    this->flash_start = (uint8_t*)FLASH_START + PAGE_SIZE;
    this->flash_pages = NO_MBR_ENTRIES-1;

    return 1;
}

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
int MicroBitFile::open(char const * const filename, uint8_t flags) {
    if(!FS_INITIALIZED() || strlen(filename) > MAX_FILENAME_LEN) return -1;
   
    mbr* m;
    int fd;
   
    //find the file, if it already exists.
    if((m = this->mbr_by_name(filename)) == NULL) {

        // File doesn't exist, try to create it.
        if((flags & MB_CREAT) != MB_CREAT || 
           (m = this->mbr_get_free()) == NULL || 
           !this->mbr_add(m, filename)) {

            // Couldn't set the MBR entry.
            return -1;
        }
   
        PRINTF("open: file created %s\n", filename);
    }
   
    //find a free FD.
    for(fd=0;fd<MAX_FD;fd++) {
        if((this->fd_table[fd].flags & MB_FD_BUSY) != MB_FD_BUSY) break;
    }
    if(fd == MAX_FD) {
        return -1;
        PRINTF("No free FDs\n");
    }
   
    //populate the fd.
    this->fd_table[fd].flags = (flags & ~(MB_CREAT)) | MB_FD_BUSY;
    this->fd_table[fd].mbr_entry = m;
    this->fd_table[fd].seek = 0;
    this->fd_table[fd].filesize = mbr_get_filesize(m);
    return fd;
}

/**
  * Close the specified file handle.
  * File handle resources are then made available for future open() calls.
  *
  * tfs_close() must be called at some point to ensure the filesize in the
  * MBR is synced with the cached value in the FD.
  *
  * @warning if tfs_close() is not called, the MBR may not be correct, l
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
int MicroBitFile::close(int fd) {
    if(!FS_INITIALIZED() ||
      (this->fd_table[fd].flags & MB_FD_BUSY) != MB_FD_BUSY) return 0;

    this->mbr_set_filesize(this->fd_table[fd].mbr_entry,
                           this->fd_table[fd].filesize);

    this->fd_table[fd].flags = 0x00;
    return 1;
}

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
int MicroBitFile::seek(int fd, int offset, uint8_t flags) {
    if(!FS_INITIALIZED() ||
      (this->fd_table[fd].flags & MB_FD_BUSY) != MB_FD_BUSY) return -1;
    
    int32_t new_pos = 0;
    int max_size = this->fd_table[fd].filesize;
   
    if(flags == MB_SEEK_SET && offset <= max_size) {
        new_pos = offset;
    }
    else if(flags == MB_SEEK_END && offset <= max_size) {
        new_pos = max_size+offset;
    }
    else if(flags == MB_SEEK_CUR && 
           (this->fd_table[fd].seek+offset) <= max_size) {
        new_pos = this->fd_table[fd].seek + offset;
    }
    else {
        PRINTF("seek range error, offset %d, flag %d, fd %dn", 
               offset, flags, fd);
        return -1;
    }  
   
    this->fd_table[fd].seek = new_pos;
    return new_pos;
}

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
int MicroBitFile::read(int fd, uint8_t* buffer, int size) {
    if(!FS_INITIALIZED() ||
      (this->fd_table[fd].flags & MB_FD_BUSY) != MB_FD_BUSY ||
      (this->fd_table[fd].flags & MB_READ) != MB_READ) return -1;
   
    // Basic algorithm:
    // Find the starting block number & offset,
    //
    //  bInd = starting block index in list
    //  ofs = offset within first block
    //
    //  while [still have bytes to read
    //   b = block number (from bInd)
    //   s = how many bytes to read from this block
    //   if(s > bytes remaining in file)
    //    s = bytes remaining in file
    //   end
    //
    //   memcpy(destination buffer, flash_start + (b * PAGE_SIZE) + ofs)
    //   seek += s
    //   bytesRead += s
    //   ofs = 0
    //   bInd++
    //  end
    //
    //  return bytesRead
    //

    int bInd = this->fd_table[fd].seek / PAGE_SIZE;
    int ofs = this->fd_table[fd].seek % PAGE_SIZE;
    int32_t filesize = this->fd_table[fd].filesize; 
    int bytesRead = 0;

    for(int sz = size;sz > 0 && this->fd_table[fd].seek<filesize;) {

        // b = block number.
        int b = mbr_get_block(this->fd_table[fd].mbr_entry, bInd);

        // s  no. bytes to read from this page.
        int s = MIN(PAGE_SIZE-ofs,sz);

        //Can't read beyond the end of the file.
        if((this->fd_table[fd].seek+s) > filesize) {
            s = filesize - this->fd_table[fd].seek;
        }

        PRINTF("Read %d bytes from block %d (%d) offset: %d\n",s,bInd,b,ofs);

        memcpy( buffer, flash_start + (PAGE_SIZE * b) + ofs, s);

        sz -= s;
        buffer += s;
        bInd++;
        ofs=0;
        this->fd_table[fd].seek += s;
        bytesRead += s;
    }

    return bytesRead;
}


/**
  * Write data to the file.
  *
  * Write from buffer, len bytes to the current seek position.
  * On each invocation to write, the seek position of the file handle
  * is incremented atomically, by the number of bytes returned.
  *
  * The cached filesize in the FD is updated on this call. Also, the
  * MBR file size is updated only if a new page(s) has been written too,
  * to reduce the number of MBR writes.
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
int MicroBitFile::write(int fd, uint8_t* buffer, int size) {
    if(!FS_INITIALIZED() ||
      (this->fd_table[fd].flags & MB_FD_BUSY) != MB_FD_BUSY ||
      (this->fd_table[fd].flags & MB_WRITE) != MB_WRITE) return -1;
   
    // Basic algorithm for writing:
    //
    // bytesWritten = 0
    //
    // while [still have bytes to write]
    //  bInd = block Index to write too
    //  ofs = ofet within block.
    //  wr = number of bytes to write
    //
    //  if [new block required]
    //   b = pop_free_block()
    //   newPages=1
    //  else
    //   b = get absolute block No. from MBR
    //  end
    //
    //  flash_write(flash_start + b + ofs, buffer, wr)
    //
    //  set cached file size if changed.
    //  bytesWritten = wr
    // end
    //
    // if [newPages]
    //  write file size to MBR
    // end
    //
    // return bytesWritten

    int bytesWritten = 0;
    int newPages=0;

    int filesize = this->fd_table[fd].filesize;
    int allocated_blocks = filesize / PAGE_SIZE; //current no. blocks assigned.
    if( (filesize % PAGE_SIZE) != 0) allocated_blocks++;

    for(int sz = size;sz > 0;) {

        int bInd = this->fd_table[fd].seek / PAGE_SIZE; // Block index.
        int ofs = this->fd_table[fd].seek % PAGE_SIZE;  // offset within block.
        int wr = MIN(PAGE_SIZE-ofs,sz);  // No. bytes to write.
        int b = 0;                       //absolute block number.

        if(bInd >= allocated_blocks) { 

           // File must increase in size, append with new block.
            if( (b = mbr_pop_free_block()) < 0) {
                break;
            }
            if(!mbr_add_block(this->fd_table[fd].mbr_entry, b)) {
                break;
            }

            allocated_blocks++;
            newPages=1;
            PRINTF("New block allocated: %d\n", b);
        }
        else {
           // Write position requires no new block allocation.
            b = mbr_get_block(this->fd_table[fd].mbr_entry, bInd);
        }

        PRINTF("Write %s block index %d, number %d, offset %d, length %d\n",
               (writeData?"data":"0x00"), bInd, b, ofs, wr);

        if(!this->flash.flash_write(this->flash_start+(PAGE_SIZE*b)+ofs,
                                    buffer,wr)) {
            break;
        }

        //set the new file length, if changed. 
        if((this->fd_table[fd].seek + wr) > this->fd_table[fd].filesize) {
            this->fd_table[fd].filesize = this->fd_table[fd].seek + wr;
        }

        sz -= wr;
        buffer += wr;
        this->fd_table[fd].seek += wr;
        bytesWritten += wr;
    }

    //change the file size if we used new pages.
    if(newPages) {
        if(!this->mbr_set_filesize(this->fd_table[fd].mbr_entry, 
                                   this->fd_table[fd].filesize)) {
            return -1;
        }
    }
    return bytesWritten;
}

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
int MicroBitFile::unlink(char const * const filename) {
    if(!FS_INITIALIZED()) return 0;
   
    // Get the mbr to remove.
    mbr* m = this->mbr_by_name(filename);
    if(m == NULL) {
        PRINTF("cannot unlink %s, not found\n", filename);
        return 0;
    }
    
    return this->mbr_remove(m);
}
