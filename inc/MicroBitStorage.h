#ifndef MICROBIT_STORAGE_H
#define MICROBIT_STORAGE_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "ManagedString.h"
#include "ErrorNo.h"

#define MICROBIT_STORAGE_MAGIC       0xCAFE

#define MICROBIT_STORAGE_BLOCK_SIZE             48
#define MICROBIT_STORAGE_KEY_SIZE               16
#define MICROBIT_STORAGE_VALUE_SIZE             MICROBIT_STORAGE_BLOCK_SIZE - MICROBIT_STORAGE_KEY_SIZE

#define MICROBIT_STORAGE_STORE_PAGE_OFFSET      19      //Use the page just below the BLE Bond Data.
#define MICROBIT_STORAGE_SCRATCH_PAGE_OFFSET    20      //Use the page just below our storage page as our scratch area.

struct KeyValuePair
{
    uint8_t key[MICROBIT_STORAGE_KEY_SIZE] = { 0 };
    uint8_t value[MICROBIT_STORAGE_VALUE_SIZE] = { 0 };
};

struct KeyValueStore
{
    uint32_t magic;
    uint32_t size;

    KeyValueStore(uint32_t magic, uint32_t size)
    {
        this->magic = magic;
        this->size = size;
    }

    KeyValueStore()
    {
        this->magic = 0;
        this->size = 0;
    }
};


/**
  * Class definition for the MicroBitStorage class.
  * This allows reading and writing of small blocks of data to FLASH memory.
  */
class MicroBitStorage
{
    /**
     * Function for copying words from one location to another.
     *
     * @param from the address to copy data from.
     * @param to the address to copy the data to.
     * @param sizeInWords the number of words to copy
     */
    void flashCopy(uint32_t* from, uint32_t* to, int sizeInWords);

    /**
     * Function for populating the scratch page with a KeyValueStore.
     *
     * @param store the KeyValueStore struct to write to the scratch page.
     */
    void scratchKeyValueStore(KeyValueStore store);

    /**
     * Function for populating the scratch page with a KeyValuePair.
     *
     * @param pair the KeyValuePair struct to write to the scratch page.
     * @param flashPointer the pointer in flash where this KeyValuePair resides. This pointer
     * is used to determine the offset into the scratch page, where the KeyValuePair should
     * be written.
     */
    void scratchKeyValuePair(KeyValuePair pair, uint32_t* flashPointer);

    public:

    /*
     * Default constructor.
     */
    MicroBitStorage();

    /*
     * Writes the given number of bytes to the address specified.
     * @param buffer the data to write.
     * @param address the location in memory to write to.
     * @param length the number of bytes to write.
     * TODO: Write this one!
     */
    int writeBytes(uint8_t *buffer, uint32_t address, int length);

    /**
     * Method for erasing a page in flash.
     * @param page_address Address of the first word in the page to be erased.
     */
    void flashPageErase(uint32_t * page_address);

    /**
     * Method for writing a word of data in flash with a value.

     * @param address Address of the word to change.
     * @param value Value to be written to flash.
     */
    void flashWordWrite(uint32_t * address, uint32_t value);

    /**
     * Places a given key, and it's corresponding value into flash at the earliest
     * available point.
     *
     * @param key the unique name that should be used as an identifier for the given data.
     * The key is presumed to be null terminated.
     * @param data a pointer to the beginning of the data to be persisted.
     *
     * @return MICROBIT_OK on success, or MICROBIT_NO_RESOURCES if our storage page is full
     */
    int put(const char* key, uint8_t* data);

    /**
     * Places a given key, and it's corresponding value into flash at the earliest
     * available point.
     *
     * @param key the unique name that should be used as an identifier for the given data.
     * @param data a pointer to the beginning of the data to be persisted.
     */
    int put(ManagedString key, uint8_t* data);

    /**
     * Retreives a KeyValuePair identified by a given key.
     *
     * @param key the unique name used to identify a KeyValuePair in flash.
     *
     * @return a pointer to a heap allocated KeyValuePair struct, this pointer will be
     * NULL if the key was not found in storage.
     *
     * @note it is up to the user to free the returned struct.
     */
    KeyValuePair* get(const char* key);

    /**
     * Retreives a KeyValuePair identified by a given key.
     *
     * @param key the unique name used to identify a KeyValuePair in flash.
     *
     * @return a pointer to a heap allocated KeyValuePair struct, this pointer will be
     * NULL if the key was not found in storage.
     *
     * @note it is up to the user to free the returned struct.
     */
    KeyValuePair* get(ManagedString key);

    /**
     * Removes a KeyValuePair identified by a given key.
     *
     * @param key the unique name used to identify a KeyValuePair in flash.
     *
     * @return MICROBIT_OK on success, or MICROBIT_NO_DATA if the given key
     * was not found in flash.
     */
    int remove(const char* key);

    /**
     * Removes a KeyValuePair identified by a given key.
     *
     * @param key the unique name used to identify a KeyValuePair in flash.
     *
     * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the given key
     * was not found in flash.
     */
    int remove(ManagedString key);

    /**
     * The size of the flash based KeyValueStore.
     *
     * @return the number of entries in the key value store
     */
    int size();
};

#endif
