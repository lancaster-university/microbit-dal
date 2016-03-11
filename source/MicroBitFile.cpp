#include <stddef.h>
#include <string.h>
#include "MicroBitFile_config.h"
#include "MicroBitFile.h"
#include "MicroBitFlash.h"

// Test if init()/mbr_init have been called.
#define FS_INITIALIZED() (this->flash_start != NULL)
#define MBR_INITIALIZED() (this->mbr_loc != NULL)

// Check if a mbr pointer is within table.
#define MBR_PTR_VALID(p) ( (p >= this->mbr_loc) && (p-this->mbr_loc)<=(this->mbr_entries-1) )

#define MIN(a,b) ((a)<(b)?(a):(b))

#define DEBUG 0
#if DEBUG
#define PRINTF(...) uBit.serial.printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*
 * Search for an MBR entry by name
 */
mbr* MicroBitFile::mbr_by_name(char const * const filename) {
 if(!MBR_INITIALIZED() || strlen(filename) > MAX_FILENAME_LEN) return NULL;

 for(int i=0;i<this->mbr_entries;++i) {
  if(strcmp(filename, this->mbr_loc[i].name) == 0) return &this->mbr_loc[i];
 }
 return NULL;
}
/*
 * Find the lowest numbered available MBR entry 
 */
mbr* MicroBitFile::mbr_get_free() {
 if(!MBR_INITIALIZED()) return NULL;

 for(int i=0;i<this->mbr_entries;++i) {
  if(MBR_IS_FREE(this->mbr_loc[i])) return &this->mbr_loc[i];
 }
 return NULL;
}

/*
 * Initialize an MBR entry with Filename, mark as unavailable, and 
 * set filesize = 0.
 */
int MicroBitFile::mbr_add(mbr* m, char const * const filename) {
 if(!MBR_INITIALIZED() || !MBR_PTR_VALID(m) ||
    !MBR_IS_FREE((*m)) || strlen(filename) > MAX_FILENAME_LEN) return 0;

 uint32_t t = MBR_BUSY;
 if(!this->flash.flash_write((uint8_t*)m->name, (uint8_t*)filename, strlen(filename)+1) ||
    !this->flash.flash_write((uint8_t*)&(m->flags), (uint8_t*)&t, sizeof(t))) return 0;

 PRINTF("Set mbr entry, filename: %s\n", filename);
 return 1;
}

/*
 * Add a data block to an MBR entry
 */
int MicroBitFile::mbr_add_block(mbr* m, uint8_t blockNo) {
 if(!MBR_INITIALIZED() || !MBR_PTR_VALID(m)) return 0;

 //find the lowest unused block entry.
 int b = 0;
 for(b=0;b<DATA_BLOCK_COUNT;b++) {
  if(m->blocks[b] == 0xFF) break;
 }
 if(b==DATA_BLOCK_COUNT) return 0;

 //okay.
 return this->flash.flash_write(&(m->blocks[b]), &blockNo, 1);
}

// ----------------------------------------------
// Return block numbers in the mbr_t entry to the mbr_free_loc->blocks list.
// Add to the beginning of the list.
//
// So if we have before:
// [ ][ ][ ][4][5][6]
//
// Inserting '1' Will then become:
// [ ][ ][1][4][5][6]
// ----------------------------------------------
int MicroBitFile::mbr_remove(mbr* m) {
 if(!MBR_INITIALIZED() || !MBR_PTR_VALID(m)) return 0;

 int used=0;    // used = no. used blocks in mbr.
 for(used=0;used<DATA_BLOCK_COUNT;used++) {
  if( m->blocks[used] == 0xFF) break;
 }

 int b = 0;     // Where to insert into free list.
 for(b=0;b<DATA_BLOCK_COUNT;++b) {
  if(this->mbr_free_loc->blocks[b] != 0x00) break;
 }

 if(used > b) { //not enough room...
  return 0;
 }
 int p = b-used;

 PRINTF("mbr removed. Name: %s, blocks: %d\n", m->filename, used);

 if(!this->flash.flash_erase_mem((uint8_t*)&this->mbr_free_loc->blocks[p],used)) return 0;

 for(int j=0;j<used;j++) {
  uint8_t bl = m->blocks[j] | MBR_FREE_BLOCK_MARKER;
  if(!this->flash.flash_write(&this->mbr_free_loc->blocks[p+j], &bl, 1)) return 0;
 }
 return this->flash.flash_erase_mem((uint8_t*)m, sizeof(mbr));
}

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
int MicroBitFile::mbr_pop_free_block() {
  if(!MBR_INITIALIZED()) return -1;

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

  //mark as unavailable.
  uint8_t write_empty = 0x00;
  uint8_t blockNumber = this->mbr_free_loc->blocks[b] &= ~(MBR_FREE_BLOCK_MARKER);
  if(!this->flash.flash_write(&this->mbr_free_loc->blocks[b], &write_empty, 1)) return -1;

  PRINTF("Free block %d from location in list %d\n", blockNumber, b);
  return blockNumber;
}

/*
 * Initialize the MBR functions, with pointers to the MBR page in flash.
 */
int MicroBitFile::mbr_init(void* mbr_location, int mbr_no) {
  if(MBR_INITIALIZED()) return 0;
  uBit.serial.printf("initializing mbr\n");
  this->mbr_free_loc = (mbr*)mbr_location;
  this->mbr_loc = (mbr*)mbr_location + 1;
  this->mbr_entries = mbr_no-1;
  return 1;
}

/*
 * Set the filesize attribute of an MBR entry 
 */
int MicroBitFile::mbr_set_filesize(mbr* m, uint32_t fz) {
  if(!MBR_INITIALIZED() || !MBR_PTR_VALID(m) || MBR_IS_FREE(*m)) return 0;

  PRINTF("set filesize: %s to %d bytes\n", m->name, fz);
  return this->flash.flash_write((uint8_t*)&m->flags, (uint8_t*)&fz, 4);
}

/*
 * Check if the Master Boot Record (MBR) table has already been initialized.
 * Initialize if not.
 */
int MicroBitFile::mbr_build() {
  if(!MBR_INITIALIZED()) return 0;

  // Only build the MBR table if not already initialized...
  if(*((uint32_t*)this->mbr_free_loc) == MAGIC_WORD) {
    uBit.serial.printf("Magic word detected, file system already built\n");
    return 1;
  }
  uBit.serial.printf("Magic word not detected, building file system\n");

  uint32_t magic = MAGIC_WORD;

  // Erase MBR Page.
  if(!this->flash.flash_erase_mem((uint8_t*)this->mbr_free_loc, PAGE_SIZE)) return 0;

  // Write Magic.
  if(!this->flash.flash_write((uint8_t*)this->mbr_free_loc->name,
                  (uint8_t*)&magic, sizeof(magic))) return 0;


  // Write Free data blocks.
  uint8_t bl[DATA_BLOCK_COUNT-1];
  for(int i=0;i<(DATA_BLOCK_COUNT-1);++i) bl[i] = i | MBR_FREE_BLOCK_MARKER;

  return this->flash.flash_write((uint8_t*)&this->mbr_free_loc->blocks, bl, DATA_BLOCK_COUNT-1);
}

/*
 * Constructor
 */
MicroBitFile::MicroBitFile() {
 uBit.serial.printf("initializing.. %d\n", this->init());
 memset(this->fd_table, 0x00, sizeof(tinyfs_fd) * MAX_FD);
}

/*
 * Init function, call mbr_init and mbr_build, store the flash memory location.
 */
int MicroBitFile::init() {
 if(FS_INITIALIZED()) return 0;

 if(!this->mbr_init((uint8_t*)FLASH_START, NO_MBR_ENTRIES)) return 0;

 if(!this->mbr_build()) return 0;

 uBit.serial.printf("initialized\n");
  
 this->flash_start = (uint8_t*)FLASH_START + PAGE_SIZE;
 this->flash_pages = NO_MBR_ENTRIES-1;
 return 1;
}

/*
 * Open a file, and obtain a file handle.
 */
int MicroBitFile::open(char const * const filename, uint8_t flags) {
 if(!FS_INITIALIZED() || strlen(filename) > MAX_FILENAME_LEN) return -1;

 mbr* m;
 int fd;

 //find, or create, the file.
 if((m = this->mbr_by_name(filename)) == NULL) {
  if((flags & MB_CREAT) != MB_CREAT || 
     (m = this->mbr_get_free()) == NULL || 
     !this->mbr_add(m, filename)) return -1;

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
 return fd;
}

/*
 * Close file handle, FD available for re-use.
 */
int MicroBitFile::close(int fd) {
 if(!FS_INITIALIZED() ||
   (this->fd_table[fd].flags & MB_FD_BUSY) != MB_FD_BUSY) return 0;

 this->fd_table[fd].flags = 0x00;
 return 1;
}

/*
 * Seek to a set position in the file
 */
int MicroBitFile::seek(int fd, int offset, uint8_t flags) {
 if(!FS_INITIALIZED() ||
   (this->fd_table[fd].flags & MB_FD_BUSY) != MB_FD_BUSY) return -1;
 
 int32_t new_pos = 0;
 int max_size = mbr_get_filesize(this->fd_table[fd].mbr_entry);

 if(flags == MB_SEEK_SET && offset <= max_size) {
   new_pos = offset;
 }
 else if(flags == MB_SEEK_END && offset <= max_size) {
   new_pos = max_size+offset;
 }
 else if(flags == MB_SEEK_CUR && (this->fd_table[fd].seek+offset) <= max_size) {
   new_pos = this->fd_table[fd].seek + offset;
 }
 else {
  PRINTF("seek range error, offset %d, flag %d, fd %dn", offset, flags, fd);
  return -1;
 }

 this->fd_table[fd].seek = new_pos;
 return new_pos;
}

// Basic algorithm:
// Find the starting block number & offset,
//
//  blockInd = starting block index in list
//  blockOffset = offset within first block
// 
//  while [still have bytes to read
//   b = block number (from blockInd)
//   s = how many bytes to read from this block
//   if(s > bytes remaining in file)
//    s = bytes remaining in file
//   end
// 
//   memcpy(destination buffer, this->flash_start + (b * PAGE_SIZE) + blockOffset)
//   seek += s
//   bytesRead += s 
//   blockOffset = 0
//   blockInd++
//  end
//
//  return bytesRead
//
int MicroBitFile::read(int fd, uint8_t* buffer, int size) {
 if(!FS_INITIALIZED() ||
   (this->fd_table[fd].flags & MB_FD_BUSY) != MB_FD_BUSY ||
   (this->fd_table[fd].flags & MB_READ) != MB_READ) return -1;

 int blockInd = this->fd_table[fd].seek / PAGE_SIZE;
 int blockOffset = this->fd_table[fd].seek % PAGE_SIZE;
 int32_t filesize = mbr_get_filesize(this->fd_table[fd].mbr_entry);
 int bytesRead = 0;

 for(int sz = size;sz > 0 && this->fd_table[fd].seek<filesize;) {

  int b = mbr_get_block(this->fd_table[fd].mbr_entry, blockInd);
  int s = MIN(PAGE_SIZE-blockOffset,sz);
  if((this->fd_table[fd].seek+s) > filesize) {
   s = filesize - this->fd_table[fd].seek;
  }

  PRINTF("Read %d bytes from block %d (%d) offset: %d\n",
	s, blockInd, b, s, blockOffset);

  memcpy( buffer, this->flash_start + (PAGE_SIZE * b) + blockOffset, s);
 
  sz -= s;
  buffer += s;
  blockInd++;
  blockOffset=0;
  this->fd_table[fd].seek += s;
  bytesRead += s;
 }

 return bytesRead;
}

// More complex than tfs_read(), because we have to account for the
// file increasing in size, and padding 0x00 if starting past current end.
// 
// allocated_blocks = current no. allocated blocks to mbr
// 
// while still data to write
//  if(seek extends beyond current file size)
//   bInd = final block in file
//   ofs = current end-offset in last block
//   wr = how many bytes to write 0x00 to
//   writeData = false (don't write data on this iteration, just fill 0x00)
//  else
//   bInd = block Index to start writing.
//   ofs = offset within block
//   wr = number of bytes to write.
//   writeData = true (write data from buffer on this iteration)
//  end
// 
//  if(bInd >= allocated_blocks)
//   b = pop_free_block()
//   add_block(b)
//  else
//   b = get_block(bInd)
//  end
// 
//  if(writeData)
//   write to(this->flash_start + PAGE_SIZE * b + ofs, buffer, wr)
//  else
//   flash_memset(this->flash_start + PAGE_SIZE + b + ofs, 0x00, wr)
//  end
// 
//  bytesWritten += wr
//  bInd++
// 
//  return bytesWritten
//  
int MicroBitFile::write(int fd, uint8_t* buffer, int size) {
 if(!FS_INITIALIZED() ||
   (this->fd_table[fd].flags & MB_FD_BUSY) != MB_FD_BUSY ||
   (this->fd_table[fd].flags & MB_WRITE) != MB_WRITE) return -1;

 int bInd; //block index
 int b; //absolute block number to write.
 int ofs; //offset within block to write.
 int wr; //no. bytes to write on a pass.
 
 int bytesWritten = 0;
 int writeData; //flag: 1=write data, 0=pad 0x00.

 int filesize = mbr_get_filesize(this->fd_table[fd].mbr_entry); 
 int origSize = filesize;

 int allocated_blocks = filesize / PAGE_SIZE; //current no. blocks assigned.
 if( (filesize % PAGE_SIZE) != 0) allocated_blocks++;

 for(int sz = size;sz > 0;) {

  if(this->fd_table[fd].seek > filesize) {
   bInd = (filesize) / PAGE_SIZE;
   ofs = (filesize) % PAGE_SIZE;
   wr = MIN(PAGE_SIZE-ofs, this->fd_table[fd].seek-filesize);
   writeData = 0;
  }
  else { //starting beyond current file end, pad 0x00.
   bInd = this->fd_table[fd].seek / PAGE_SIZE;
   ofs = this->fd_table[fd].seek % PAGE_SIZE;
   wr = MIN(PAGE_SIZE-ofs,sz);
   writeData = 1;
  }
 
  if(bInd >= allocated_blocks) { // Get the block number, or allocate one.
   if( (b = this->mbr_pop_free_block()) < 0 || 
      ! this->mbr_add_block(this->fd_table[fd].mbr_entry, b) ) break;

   allocated_blocks++;
   PRINTF("New block allocated: %d\n", b);
  }
  else {
   b = mbr_get_block(this->fd_table[fd].mbr_entry, bInd);
  } 

  PRINTF("Write %s block index %d, number %d, offset %d, length %d\n",
         (writeData?"data":"0x00"), bInd, b, ofs, wr);

  if(writeData) { //write data from buffer.
   if(!this->flash.flash_write( this->flash_start + (PAGE_SIZE * b) + ofs, buffer, wr)) break;
   if((this->fd_table[fd].seek + wr) > filesize) { //set the new file length
    filesize = this->fd_table[fd].seek + wr;
   }

   sz -= wr; 
   buffer += wr;
   this->fd_table[fd].seek += wr;
  }
  else { //write 0x00 to pad.
   if(!this->flash.flash_memset( this->flash_start + (PAGE_SIZE * b) + ofs, 0x00, wr)) break;
   filesize += wr; //set the new file length.
  }

  bytesWritten += wr;
 }

 if(origSize != filesize) {
   if(!this->mbr_set_filesize(this->fd_table[fd].mbr_entry, filesize))return -1;
 }
 return bytesWritten;
}

/*
 * Delete a file. This deletes atomically.
 */ 
int MicroBitFile::unlink(char const * const filename) {
 if(!FS_INITIALIZED()) return 0;

 mbr* m = this->mbr_by_name(filename);
 if(m == NULL) {
  PRINTF("cannot unlink %s, not found\n", filename);
  return 0;
 }
 
 return this->mbr_remove(m);
}
