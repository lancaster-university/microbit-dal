#ifndef MICROBIT_SERIAL_H
#define MICROBIT_SERIAL_H

#include "mbed.h"
#include "ManagedString.h"
#include "MicroBitImage.h"

#define MICROBIT_SERIAL_DEFAULT_BAUD_RATE 115200
#define MICROBIT_SERIAL_BUFFER_SIZE 20

#define MICROBIT_SERIAL_DEFAULT_EOF '\n'

/**
  * Class definition for MicroBitSerial.
  *
  * Represents an instance of Serial which accepts micro:bit specific data types.
  */
class MicroBitSerial : public Serial
{
    ssize_t readChars(void* buffer, size_t length, char eof = MICROBIT_SERIAL_DEFAULT_EOF);
    
    public:
    
    /**
      * Constructor. 
      * Create an instance of MicroBitSerial
      * @param sda the Pin to be used for SDA
      * @param scl the Pin to be used for SCL
      * Example:
      * @code 
      * MicroBitSerial serial(USBTX, USBRX);
      * @endcode
      */
    MicroBitSerial(PinName tx, PinName rx);
    
    /**
      * Sends a managed string over serial.
      *
      * @param s the ManagedString to send
      *
      * Example:
      * @code 
      * uBit.serial.printString("abc123");
      * @endcode
      */
    void sendString(ManagedString s);
    
    /**
      * Reads a ManagedString from serial
      *
      * @param len the buffer size for the string, default is defined by MICROBIT_SERIAL_BUFFER_SIZE
      *
      * Example:
      * @code 
      * uBit.serial.readString();
      * @endcode
      *
      * @note this member function will wait until either the buffer is full, or a \n is received
      */
    ManagedString readString(int len = MICROBIT_SERIAL_BUFFER_SIZE);
    
    /**
      * Sends a MicroBitImage over serial in csv format.
      *
      * @param i the instance of MicroBitImage you would like to send.
      *
      * Example:
      * @code 
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart);
      * uBit.serial.sendImage(i);
      * @endcode
      */
    void sendImage(MicroBitImage i);

    /**
      * Reads a MicroBitImage over serial, in csv format.
      *
      *
      * @return a MicroBitImage with the format described over serial
      *
      * Example:
      * @code 
      * MicroBitImage i = uBit.serial.readImage(2,2);
      * @endcode
      *
      * Example Serial Format:
      * @code 
      * 0,10x0a0,10x0a // 0x0a is a LF terminal which is used as a delimeter
      * @endcode
      * @note this will finish once the dimensions are met.
      */
    MicroBitImage readImage(int width, int height);
    
    /**
      * Sends the current pixel values, byte-per-pixel, over serial
      *
      * Example:
      * @code 
      * uBit.serial.sendDisplayState();
      * @endcode
      */
    void sendDisplayState();
    
    /**
      * Reads pixel values, byte-per-pixel, from serial, and sets the display.
      *
      * Example:
      * @code 
      * uBit.serial.readDisplayState();
      * @endcode
      */
    void readDisplayState();
    
};

#endif
