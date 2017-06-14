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

#ifndef MICROBIT_FILE_H
#define MICROBIT_FILE_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitFileSystem.h"
#include "ManagedString.h"

#define READ                            MB_READ
#define WRITE                           MB_WRITE
#define READ_AND_WRITE                  READ | WRITE
#define CREATE                          MB_CREAT

class MicroBitFile
{

    int fileHandle;
    ManagedString fileName;

    public:

    /**
      * Creates an instance of a MicroBitFile and creates a new file if required.
      *
      * @param fileName the name of the file to create/open.
      *
      * @param mode One of: READ, WRITE, READ_AND_WRITE. Defaults to READ_AND_WRITE.
      */
    MicroBitFile(ManagedString fileName, int mode = READ | WRITE | CREATE);

    /**
      * Seeks to a position in this MicroBitFile instance from the beginning of the file.
      *
      * @param offset the number of bytes to seek, relative to the beginning of the file.
      *
      * @return the new seek position, MICROBIT_NOT_SUPPORTED if the current file handle is invalid,
      *         MICROBIT_INVALID_PARAMETER if the given offset is negative.
      */
    int setPosition(int position);

    /**
      * Returns the current position of the seek head for the current file.
      *
      * @return the new seek position, MICROBIT_NOT_SUPPORTED if the current file handle is invalid.
      */
    int getPosition();

    /**
      * Writes the given bytes to this MicroBitFile instance at the current position.
      *
      * @param bytes a pointer to the bytes to write to this file.
      *
      * @param len the number of bytes to write.
      *
      * @return The number of bytes written, MICROBIT_NOT_SUPPORTED if the current file
      *         handle is invalid, MICROBIT_INVALID_PARAMETER if bytes is invalid, or
      *         len is negative.
      */
    int write(const char *bytes, int len);

    /**
      * Writes the given ManagedString to this MicroBitFile instance at the current position.
      *
      * @param s The ManagedString to write to this file.
      *
      * @return The number of bytes written, MICROBIT_NOT_SUPPORTED if the current file
      *         handle is invalid, MICROBIT_INVALID_PARAMETER if bytes is invalid, or
      *         len is negative.
      */
    int write(ManagedString s);

    /**
      * Reads a single character from the file at the current position.
      *
      * @return the character, or MICROBIT_NOT_SUPPORTED if the current file handle
      *         is invalid.
      */
    int read();

    /**
      * Reads from the file into a given buffer.
      *
      * @param buffer a pointer to the buffer where data will be stored.
      *
      * @param size the number of bytes that can be safely stored in the buffer.
      *
      * @return the number of bytes read, or MICROBIT_INVALID_PARAMETER if buffer is
      *         invalid, or the size given is less than 0.
      */
    int read(char *buffer, int size);

    /**
      * Reads from the current MicroBitFile into a given buffer.
      *
      * @param size the number of bytes to be read from the file.
      *
      * @return a ManagedString containing the requested bytes, oran empty ManagedString
      *         on error.
      */
    ManagedString read(int size);

    /**
      * Removes this MicroBitFile from the MicroBitFileSystem.
      *
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if the given filename
      *         does not exist, MICROBIT_CANCELLED if something went wrong.
      */
    int remove();

    /**
      * Seeks to the end of the file, and appends the given ManagedString to this MicroBitFile instance.
      *
      * @param bytes a pointer to the bytes to write to this file.
      *
      * @param len the number of bytes to write.
      *
      * @return The number of bytes written, MICROBIT_NOT_SUPPORTED if the current file
      *         handle is invalid or this file was not opened in WRITE mode.
      */
    int append(const char *bytes, int len);

    /**
      * Seeks to the end of the file, and appends the given ManagedString to this MicroBitFile instance.
      *
      * @param s The ManagedString to write to this file.
      *
      * @return The number of bytes written, MICROBIT_NOT_SUPPORTED if the current file
      *         handle is invalid or this file was not opened in WRITE mode.
      */
    int append(ManagedString s);

    /**
     * Determines if this MicroBitFile instance refers to a valid, open file.
     *
     * @return true if this file is valid, false otherwise.
     *
     */
    bool isValid();

    /**
      * Returns the handle used by this MicroBitFile instance.
      *
      * @note This member function will also inform the user of any errors encountered
      *       during the opening of this MicroBitFile. At open, the handle is set
      *       to the return value from MicroBitFileSystem.open().
      */
    int getHandle();

    /**
      * Closes this MicroBitFile instance
      *
      * @return MICROBIT_OK on success, MICROBIT_NOT_SUPPORTED if the file handle
      *         is invalid.
      *
      * @note MicroBitFiles are opened at construction and are implicitly closed at
      *       destruction. They can be closed explicitly using this member function.
      */
    int close();

    /**
     * Writes back all state associated with the given file to FLASH memory,
     * leaving the file open.
     *
     * @return MICROBIT_OK on success, MICROBIT_NOT_SUPPORTED if the file system has not
     *         been initialised or if this file is invalid.
     */
    int flush();

    /**
      * Destructor for MicroBitFile. Implicitly closes the current file.
      */
    ~MicroBitFile();
};

#endif
