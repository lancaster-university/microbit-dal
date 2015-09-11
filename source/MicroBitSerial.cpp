#include "mbed.h"
#include "MicroBit.h"

/**
  * Constructor. 
  * Create an instance of MicroBitSerial
  *
  * @param tx the Pin to be used for transmission
  * @param rx the Pin to be used for receiving data
  *
  * Example:
  * @code 
  * MicroBitSerial serial(USBTX, USBRX);
  * @endcode
  * @note the default baud rate is 115200
  */
MicroBitSerial::MicroBitSerial(PinName tx, PinName rx) : Serial(tx,rx)
{
    this->baud(MICROBIT_SERIAL_DEFAULT_BAUD_RATE);
}

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
void MicroBitSerial::sendString(ManagedString s)
{   
    const int len = s.length();
    const char *data = s.toCharArray();
    
    Serial::write(data,len);   
}

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
ManagedString MicroBitSerial::readString(int len)
{   
    if(len < 3)
        len = 3;

    char buffer[len];
    
    memset(buffer, 0, sizeof(buffer));
    
    int length = readChars(buffer,len); 
    
    if(length == 0)
        return ManagedString();
    
    //add in a null terminator so bad things don't happen with ManagedString
    buffer[length] = '\0';
    
    return ManagedString(buffer);   
}

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
void MicroBitSerial::sendImage(MicroBitImage i)
{
    sendString(i.toString());
}


/**
  * Reads a MicroBitImage over serial, in csv format.
  *
  *
  * @return a MicroBitImage with the format described over serial
  *
  * Example:
  * @code 
  * MicroBitImage i = uBit.serial.readImage(5,5);
  * @endcode
  *
  * Example Serial Format:
  * @code 
  * 0,10x0a0,10x0a // 0x0a is a LF terminal which is used as a delimeter
  * @endcode
  * @note this will finish once the dimensions are met.
  */
MicroBitImage MicroBitSerial::readImage(int width, int height)
{
    int rowLength = width * 2;
    int len = rowLength * height;
    
    char buffer[len + 1];
    
    memset(buffer, 0, sizeof(buffer));
    
    //add in a null terminator so bad things don't happen with MicroBitImage
    buffer[len] = '\0';
    
    for(int i = 0; i < height; i++)
    {
        int offset = i * rowLength;
        
        readChars(&buffer[offset],len - offset);
        
        buffer[(offset + rowLength) - 1] = '\n'; 
    }
    
    return MicroBitImage(buffer);
}

/**
  * Sends the current pixel values, byte-per-pixel, over serial
  *
  * Example:
  * @code 
  * uBit.serial.sendDisplayState();
  * @endcode
  */
void MicroBitSerial::sendDisplayState()
{
    for(int i = 0; i < MICROBIT_DISPLAY_HEIGHT; i++)
        for(int j = 0; j < MICROBIT_DISPLAY_WIDTH; j++)
            _putc(uBit.display.image.getPixelValue(j,i));
}

/**
  * Reads pixel values, byte-per-pixel, from serial, and sets the display.
  *
  * Example:
  * @code 
  * uBit.serial.readDisplayState();
  * @endcode
  */
void MicroBitSerial::readDisplayState()
{
    for(int i = 0; i < MICROBIT_DISPLAY_HEIGHT; i++)
        for(int j = 0; j < MICROBIT_DISPLAY_WIDTH; j++)
        {
            int c = _getc();
            uBit.display.image.setPixelValue(j,i,c);
        }
}

ssize_t MicroBitSerial::readChars(void* buffer, size_t length, char eof) {
    
    char* ptr = (char*)buffer;
    char* end = ptr + length;
    
    int eofAscii = (int)eof;
    
    while (ptr != end) {
        
        int c = _getc();
        
        //check EOF
        if (c == eofAscii) 
            break;
        
        //store the character    
        *ptr++ = c;    
    }      
    
    return ptr - (const char*)buffer;
}


