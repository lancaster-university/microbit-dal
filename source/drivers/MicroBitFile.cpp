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

#include "MicroBitFile.h"
#include "ErrorNo.h"

/**
  * Creates an instance of a MicroBitFile and creates a new file if required.
  *
  * @param fileName the name of the file to create/open.
  *
  * @param mode One of: READ, WRITE, READ_AND_WRITE. Defaults to READ_AND_WRITE.
  */
MicroBitFile::MicroBitFile(ManagedString fileName, int mode)
{
    this->fileName = fileName;

    MicroBitFileSystem* fs;

    if(MicroBitFileSystem::defaultFileSystem == NULL)
        fs = new MicroBitFileSystem();
    else
        fs = MicroBitFileSystem::defaultFileSystem;

    fileHandle = fs->open(fileName.toCharArray(), mode);
}

/**
  * Seeks to a position in this MicroBitFile instance from the beginning of the file.
  *
  * @param offset the number of bytes to seek, relative to the beginning of the file.
  *
  * @return the new seek position, MICROBIT_NOT_SUPPORTED if the current file handle is invalid,
  *         MICROBIT_INVALID_PARAMETER if the given offset is negative.
  */
int MicroBitFile::setPosition(int offset)
{
    if(fileHandle < 0)
        return MICROBIT_NOT_SUPPORTED;

    if(offset < 0)
        return MICROBIT_INVALID_PARAMETER;

    return MicroBitFileSystem::defaultFileSystem->seek(fileHandle, offset, MB_SEEK_SET);
}

/**
  * Returns the current position of the seek head for the current file.
  *
  * @return the new seek position, MICROBIT_NOT_SUPPORTED if the current file handle is invalid.
  */
int MicroBitFile::getPosition()
{
    if(fileHandle < 0)
        return MICROBIT_NOT_SUPPORTED;

    return MicroBitFileSystem::defaultFileSystem->seek(fileHandle, 0, MB_SEEK_CUR);
}

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
int MicroBitFile::write(const char *bytes, int len)
{
    if(fileHandle < 0)
        return MICROBIT_NOT_SUPPORTED;

    return MicroBitFileSystem::defaultFileSystem->write(fileHandle, (uint8_t*)bytes, len);
}

/**
  * Writes the given ManagedString to this MicroBitFile instance at the current position.
  *
  * @param s The ManagedString to write to this file.
  *
  * @return The number of bytes written, MICROBIT_NOT_SUPPORTED if the current file
  *         handle is invalid, MICROBIT_INVALID_PARAMETER if bytes is invalid, or
  *         len is negative.
  */
int MicroBitFile::write(ManagedString s)
{
    return write(s.toCharArray(), s.length());
}

/**
  * Reads a single character from the file at the current position.
  *
  * @return the character, or MICROBIT_NOT_SUPPORTED if the current file handle
  *         is invalid.
  */
int MicroBitFile::read()
{
    if(fileHandle < 0)
        return MICROBIT_NOT_SUPPORTED;

    char c[1];

    int ret = read(c,1);

    if(ret < 0)
        return ret;

    return c[0];
}

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
int MicroBitFile::read(char *buffer, int size)
{
    if(fileHandle < 0)
        return MICROBIT_NOT_SUPPORTED;

    if(size < 0 || buffer == NULL)
        return MICROBIT_INVALID_PARAMETER;

    return MicroBitFileSystem::defaultFileSystem->read(fileHandle, (uint8_t*)buffer, size);
}

/**
  * Reads from the current MicroBitFile into a given buffer.
  *
  * @param size the number of bytes to be read from the file.
  *
  * @return a ManagedString containing the requested bytes, oran empty ManagedString
  *         on error.
  */
ManagedString MicroBitFile::read(int size)
{
    char buff[size + 1];

    buff[size] = 0;

    int ret = read(buff, size);

    if(ret < 0)
        return ManagedString();

    return ManagedString(buff,ret);
}

/**
  * Removes this MicroBitFile from the MicroBitFileSystem.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if the given filename
  *         does not exist, MICROBIT_CANCELLED if something went wrong.
  */
int MicroBitFile::remove()
{
    if(fileHandle < 0)
        return MICROBIT_NOT_SUPPORTED;

    int ret = MicroBitFileSystem::defaultFileSystem->remove(fileName.toCharArray());

    if(ret < 0)
        return ret;

    fileHandle = MICROBIT_NOT_SUPPORTED;

    return ret;
}

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
int MicroBitFile::append(const char *bytes, int len)
{
    if(fileHandle < 0)
        return MICROBIT_NOT_SUPPORTED;

    int ret =  MicroBitFileSystem::defaultFileSystem->seek(fileHandle, 0, MB_SEEK_END);

    if(ret < 0)
        return ret;

    return write(bytes,len);
}

/**
  * Seeks to the end of the file, and appends the given ManagedString to this MicroBitFile instance.
  *
  * @param s The ManagedString to write to this file.
  *
  * @return The number of bytes written, MICROBIT_NOT_SUPPORTED if the current file
  *         handle is invalid or this file was not opened in WRITE mode.
  */
int MicroBitFile::append(ManagedString s)
{
    return append(s.toCharArray(), s.length());
}

/**
  * Determines if this MicroBitFile instance refers to a valid, open file.
  *
  * @return true if this file is valid, false otherwise.
  *
  */
bool MicroBitFile::isValid()
{
    return fileHandle >= 0;
}

/**
  * Returns the handle used by this MicroBitFile instance.
  *
  * @note This member function will also inform the user of any errors encountered
  *       during the opening of this MicroBitFile. At open, the handle is set
  *       to the return value from MicroBitFileSystem.open().
  */
int MicroBitFile::getHandle()
{
    return fileHandle;
}

/**
  * Closes this MicroBitFile instance
  *
  * @return MICROBIT_OK on success, MICROBIT_NOT_SUPPORTED if the file handle
  *         is invalid.
  *
  * @note MicroBitFiles are opened at construction and are implicitly closed at
  *       destruction. They can be closed explicitly using this member function.
  */
int MicroBitFile::close()
{
    if(fileHandle < 0)
        return MICROBIT_NOT_SUPPORTED;

    int ret = MicroBitFileSystem::defaultFileSystem->close(fileHandle);

    if(ret < 0)
        return ret;

    fileHandle = MICROBIT_NO_RESOURCES;

    return ret;
}

/**
 * Writes back all state associated with the given file to FLASH memory,
 * leaving the file open.
 *
 * @return MICROBIT_OK on success, MICROBIT_NOT_SUPPORTED if the file system has not
 *         been initialised or if this file is invalid.
 */
int MicroBitFile::flush()
{
    if(fileHandle < 0)
        return MICROBIT_NOT_SUPPORTED;

    return MicroBitFileSystem::defaultFileSystem->flush(fileHandle);
}

/**
  * Destructor for MicroBitFile. Implicitly closes the current file.
  */
MicroBitFile::~MicroBitFile()
{
    close();
}
