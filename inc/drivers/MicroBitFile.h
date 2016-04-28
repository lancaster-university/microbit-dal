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
    MicroBitFile(ManagedString fileName, int mode = READ_AND_WRITE);

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
      * Seeks to the end of the file, and appends the given character to this MicroBitFile instance.
      *
      * @param c The character to write to this file.
      */
    void operator+=(const char c);

    /**
      * Seeks to the end of the file, and appends the given sequence of characters to this MicroBitFile instance.
      *
      * @param s The sequence of characters to write to this file.
      *
      */
    void operator+=(const char* s);

    /**
      * Seeks to the end of the file, and appends the given ManagedString to this MicroBitFile instance.
      *
      * @param s The ManagedString to write to this file.
      *
      */
    void operator+=(ManagedString& s);

    /**
      * Returns the handle used by this MicroBitFile instance.
      *
      * @note This member function will also inform the user of any errors encountered
      *       during the opening of this MicroBitFile. At open, the handle is set
      *       to the return value from MicroBitFileSystem.open().
      */
    int getHandle();

    /**
      * Opens this MicroBitFile instance if the file has previously been closed.
      *
      * @param mode One of: READ, WRITE, READ_AND_WRITE. Defaults to READ_AND_WRITE.
      *
      * @return MICROBIT_OK on success, MICROBIT_NOT_SUPPORTED if the file is already open,
      *         MICROBIT_INVALID_PARAMETER if the filename is too large, MICROBIT_NO_RESOURCES
      *         if the file system is full, or memory could not be allocated..
      *
      * @note MicroBitFiles are opened at construction and are implicitly closed at
      *       destruction. They can be closed explicitly using the close() member function.
      */
    int open(int mode = READ_AND_WRITE);

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
      * Destructor for MicroBitFile. Implicitly closes the current file.
      */
    ~MicroBitFile();
};

#endif
