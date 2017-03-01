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

/**
  * Class definition for the MicroBitStorage class.
  * This allows reading and writing of FLASH memory.
  */

#include "MicroBitConfig.h"
#include "MicroBitStorage.h"
#include "MicroBitFlash.h"
#include "MicroBitCompat.h"


/**
  * Default constructor.
  *
  * Creates an instance of MicroBitStorage which acts like a KeyValueStore
  * that allows the retrieval, addition and deletion of KeyValuePairs.
  */
MicroBitStorage::MicroBitStorage()
{
    //initialise our magic block, if required.
    size();
}

/**
  * Method for erasing a page in flash.
  *
  * @param page_address Address of the first word in the page to be erased.
  */
void MicroBitStorage::flashPageErase(uint32_t * page_address)
{
    MicroBitFlash flash;
    flash.erase_page(page_address);
}

/**
  * Function for copying words from one location to another.
  *
  * @param from the address to copy data from.
  *
  * @param to the address to copy the data to.
  *
  * @param sizeInWords the number of words to copy
  */
void MicroBitStorage::flashCopy(uint32_t* from, uint32_t* to, int sizeInWords)
{
    MicroBitFlash flash;
    flash.flash_burn(to, from, sizeInWords);
}

/**
  * Method for writing a word of data in flash with a value.
  *
  * @param address Address of the word to change.
  *
  * @param value Value to be written to flash.
  */
void MicroBitStorage::flashWordWrite(uint32_t * address, uint32_t value)
{
    flashCopy(&value, address, 1);
}

/**
  * Function for populating the scratch page with a KeyValueStore.
  *
  * @param store the KeyValueStore struct to write to the scratch page.
  */
void MicroBitStorage::scratchKeyValueStore(KeyValueStore store)
{
    //calculate our various offsets
    uint32_t *s = (uint32_t *) &store;
    uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
    uint32_t *scratchPointer = (uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_SCRATCH_PAGE_OFFSET));
    int wordsToWrite = sizeof(KeyValueStore) / 4;

    flashCopy(s, scratchPointer, wordsToWrite);
}

/**
  * Function for populating the scratch page with a KeyValuePair.
  *
  * @param pair the KeyValuePair struct to write to the scratch page.
  *
  * @param flashPointer the pointer in flash where this KeyValuePair resides. This pointer
  * is used to determine the offset into the scratch page, where the KeyValuePair should
  * be written.
  */
void MicroBitStorage::scratchKeyValuePair(KeyValuePair pair, uint32_t* flashPointer)
{
    //we can only write using words
    uint32_t *p = (uint32_t *) &pair;

    //calculate our various offsets
    uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
    uint32_t pg_num  = NRF_FICR->CODESIZE - MICROBIT_STORAGE_STORE_PAGE_OFFSET;

    uint32_t *scratchPointer = (uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_SCRATCH_PAGE_OFFSET));
    uint32_t *flashBlockPointer = (uint32_t *)(pg_size * pg_num);

    uint32_t flashPointerOffset = flashPointer - flashBlockPointer;

    scratchPointer += flashPointerOffset;

    //KeyValuePair is word aligned...
    int wordsToWrite = sizeof(KeyValuePair) / 4;

    flashCopy(p, scratchPointer, wordsToWrite);
}

/**
  * Places a given key, and it's corresponding value into flash at the earliest
  * available point.
  *
  * @param key the unique name that should be used as an identifier for the given data.
  *            The key is presumed to be null terminated.
  *
  * @param data a pointer to the beginning of the data to be persisted.
  *
  * @param dataSize the size of the data to be persisted
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if the key or size is too large,
  *         MICROBIT_NO_RESOURCES if the storage page is full
  */
int MicroBitStorage::put(const char *key, uint8_t *data, int dataSize)
{
    KeyValuePair pair = KeyValuePair();

    int keySize = strlen(key) + 1;

    if(keySize > (int)sizeof(pair.key) || dataSize > (int)sizeof(pair.value) || dataSize < 0)
        return MICROBIT_INVALID_PARAMETER;

    KeyValuePair *currentValue = get(key);

    int upToDate = currentValue && (memcmp(currentValue->value, data, dataSize) == 0);

    if(currentValue)
        delete currentValue;

    if(upToDate)
        return MICROBIT_OK;

    memcpy(pair.key, key, keySize);
    memcpy(pair.value, data, dataSize);

    //calculate our various offsets.
    uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
    uint32_t *flashPointer = (uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_STORE_PAGE_OFFSET));
    uint32_t *flashBlockPointer = flashPointer;
    uint32_t *scratchPointer = (uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_SCRATCH_PAGE_OFFSET));

    uint32_t kvStoreSize = sizeof(KeyValueStore) / 4;
    uint32_t kvPairSize = sizeof(KeyValuePair) / 4;

    int storeSize = size();

    //our KeyValueStore struct is always at 0
    flashPointer += kvStoreSize;

    KeyValuePair storedPair = KeyValuePair();

    int found = 0;

    //erase our scratch page
    flashPageErase(scratchPointer);

    //iterate through key value pairs in flash, writing them to the scratch page.
    for(int i = 0; i < storeSize; i++)
    {
        memcpy(&storedPair, flashPointer, sizeof(KeyValuePair));

        //check if the keys match...
        if(strcmp((char *)storedPair.key, (char *)pair.key) == 0)
        {
            found = 1;
            //scratch our KeyValueStore struct so that it is preserved.
            scratchKeyValueStore(KeyValueStore(MICROBIT_STORAGE_MAGIC, storeSize));
            scratchKeyValuePair(pair, flashPointer);
        }
        else
        {
            scratchKeyValuePair(storedPair, flashPointer);
        }

        flashPointer += kvPairSize;
    }

    if(!found)
    {
        //if we haven't got a match for the key, check we can add a new KeyValuePair
        if(storeSize == (int)((pg_size - kvStoreSize) / MICROBIT_STORAGE_BLOCK_SIZE))
            return MICROBIT_NO_RESOURCES;

        storeSize += 1;

        //scratch our updated values.
        scratchKeyValueStore(KeyValueStore(MICROBIT_STORAGE_MAGIC, storeSize));
        scratchKeyValuePair(pair, flashPointer);
    }

    //erase our storage page
    flashPageErase((uint32_t *)flashBlockPointer);

    //copy from scratch to storage.
    flashCopy((uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_SCRATCH_PAGE_OFFSET)), flashBlockPointer, kvStoreSize + (storeSize * kvPairSize));

    return MICROBIT_OK;
}

/**
  * Places a given key, and it's corresponding value into flash at the earliest
  * available point.
  *
  * @param key the unique name that should be used as an identifier for the given data.
  *
  * @param data a pointer to the beginning of the data to be persisted.
  *
  * @param dataSize the size of the data to be persisted
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if the key or size is too large,
  *         MICROBIT_NO_RESOURCES if the storage page is full
  */
int MicroBitStorage::put(ManagedString key, uint8_t* data, int dataSize)
{
    return put((char *)key.toCharArray(), data, dataSize);
}

/**
  * Retreives a KeyValuePair identified by a given key.
  *
  * @param key the unique name used to identify a KeyValuePair in flash.
  *
  * @return a pointer to a heap allocated KeyValuePair struct, this pointer will be
  *         NULL if the key was not found in storage.
  *
  * @note it is up to the user to free memory after use.
  */
KeyValuePair* MicroBitStorage::get(const char* key)
{
    //calculate our offsets for our storage page
    uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
    uint32_t pg_num  = NRF_FICR->CODESIZE - MICROBIT_STORAGE_STORE_PAGE_OFFSET;

    uint32_t *flashBlockPointer = (uint32_t *)(pg_size * pg_num);

    int storeSize = size();

    //we haven't got anything stored, so return...
    if(storeSize == 0)
        return NULL;

    //our KeyValueStore struct is always at 0
    flashBlockPointer += sizeof(KeyValueStore) / 4;

    KeyValuePair *pair = new KeyValuePair();

    int i;

    //iterate through flash until we have a match, or drop out.
    for(i = 0; i < storeSize; i++)
    {
        memcpy(pair, flashBlockPointer, sizeof(KeyValuePair));

        if(strcmp(key,(char *)pair->key) == 0)
            break;

        flashBlockPointer += sizeof(KeyValuePair) / 4;
    }

    //clean up
    if(i == storeSize)
    {
        delete pair;
        return NULL;
    }

    return pair;
}

/**
  * Retreives a KeyValuePair identified by a given key.
  *
  * @param key the unique name used to identify a KeyValuePair in flash.
  *
  * @return a pointer to a heap allocated KeyValuePair struct, this pointer will be
  *         NULL if the key was not found in storage.
  *
  * @note it is up to the user to free memory after use.
  */
KeyValuePair* MicroBitStorage::get(ManagedString key)
{
    return get((char *)key.toCharArray());
}

/**
  * Removes a KeyValuePair identified by a given key.
  *
  * @param key the unique name used to identify a KeyValuePair in flash.
  *
  * @return MICROBIT_OK on success, or MICROBIT_NO_DATA if the given key
  *         was not found in flash.
  */
int MicroBitStorage::remove(const char* key)
{
    //calculate our various offsets
    uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
    uint32_t *flashPointer = (uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_STORE_PAGE_OFFSET));
    uint32_t *flashBlockPointer = flashPointer;
    uint32_t *scratchPointer = (uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_SCRATCH_PAGE_OFFSET));

    uint32_t kvStoreSize = sizeof(KeyValueStore) / 4;
    uint32_t kvPairSize = sizeof(KeyValuePair) / 4;

    int storeSize = size();

    //if we have no data, we have nothing to do.
    if(storeSize == 0)
        return MICROBIT_NO_DATA;

    //our KeyValueStore struct is always at 0
    flashPointer += kvStoreSize;
    scratchPointer += kvStoreSize;

    KeyValuePair storedPair = KeyValuePair();

    int found = 0;

    //set up our scratch area
    flashPageErase(scratchPointer);

    //iterate through our flash copy pairs to scratch, unless there is a key patch
    for(int i = 0; i < storeSize; i++)
    {
        memcpy(&storedPair, flashPointer, sizeof(KeyValuePair));

        //if we have a match, don't increment our scratchPointer
        if(strcmp((char *)storedPair.key, (char *)key) == 0)
        {
            found = 1;
            //write our new KeyValueStore data
            scratchKeyValueStore(KeyValueStore(MICROBIT_STORAGE_MAGIC, storeSize - 1));
        }
        else
        {
            //otherwise copy the KeyValuePair from our storage page.
            flashCopy(flashPointer, scratchPointer, sizeof(KeyValuePair) / 4);
            scratchPointer += sizeof(KeyValuePair) / 4;
        }

        flashPointer += sizeof(KeyValuePair) / 4;
    }

    //if we haven't got a match, write our old KeyValueStore struct
    if(!found)
    {
        scratchKeyValueStore(KeyValueStore(MICROBIT_STORAGE_MAGIC, storeSize));
        return MICROBIT_NO_DATA;
    }

    //copy scratch to our storage page
    flashPageErase((uint32_t *)flashBlockPointer);
    flashCopy((uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_SCRATCH_PAGE_OFFSET)), flashBlockPointer, kvStoreSize + (storeSize * kvPairSize));

    return MICROBIT_OK;
}

/**
  * Removes a KeyValuePair identified by a given key.
  *
  * @param key the unique name used to identify a KeyValuePair in flash.
  *
  * @return MICROBIT_OK on success, or MICROBIT_NO_DATA if the given key
  *         was not found in flash.
  */
int MicroBitStorage::remove(ManagedString key)
{
    return remove((char *)key.toCharArray());
}

/**
  * The size of the flash based KeyValueStore.
  *
  * @return the number of entries in the key value store
  */
int MicroBitStorage::size()
{
    uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
    uint32_t pg_num  = NRF_FICR->CODESIZE - MICROBIT_STORAGE_STORE_PAGE_OFFSET;

    uint32_t *flashBlockPointer = (uint32_t *)(pg_size * pg_num);

    KeyValueStore store = KeyValueStore();

    //read our data!
    memcpy(&store, flashBlockPointer, sizeof(KeyValueStore));

    //if we haven't used flash before, we need to configure it
    if(store.magic != MICROBIT_STORAGE_MAGIC)
    {
        store.magic = MICROBIT_STORAGE_MAGIC;
        store.size = 0;

        //erase the scratch page and write our new KeyValueStore
        flashPageErase((uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_SCRATCH_PAGE_OFFSET)));
        scratchKeyValueStore(store);

        //erase flash, and copy the scratch page over
        flashPageErase((uint32_t *)flashBlockPointer);
        flashCopy((uint32_t *)(pg_size * (NRF_FICR->CODESIZE - MICROBIT_STORAGE_SCRATCH_PAGE_OFFSET)), flashBlockPointer, pg_size/4);
    }

    return store.size;
}
