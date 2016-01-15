#ifndef MICROBIT_SERIAL_H
#define MICROBIT_SERIAL_H

#include "mbed.h"
#include "ManagedString.h"
#include "MicroBitImage.h"

#define MICROBIT_SERIAL_DEFAULT_BAUD_RATE   115200
#define MICROBIT_SERIAL_DEFAULT_BUFFER_SIZE 20

#define MICROBIT_SERIAL_DEFAULT_DELIM       "\0\n"

#define MICROBIT_SERIAL_EVT_FIN_RCV         1
#define MICROBIT_SERIAL_EVT_FIN_TX          2

#define MICROBIT_SERIAL_STATE_IN_USE        1

/**
  * Class definition for MicroBitSerial.
  *
  * Represents an instance of Serial which accepts micro:bit specific data types.
  */
class MicroBitSerial : public Serial
{
    static uint8_t status;
    ManagedString *delimeters;
    char *buffer;
    char *currentChar;
    uint8_t stringLength;

    /**
      * An internal interrupt callback for MicroBitSerial.
      *
      * Each time an RX interrupt occurs, a buffer is filled. When the
      * buffer is full, an event is fired which unblocks a waiting fiber,
      * which then handles the buffer.
      */
    void dataReceived();

    /**
      * An internal interrupt callback for MicroBitSerial.
      *
      * Each time the TX buffer is empty, this method is triggered.
      */
    void dataWritten();

    /**
      * An internal method to configure an interrupt on TX buffer empty.
      *
      * On entry to this method, a flag is set which locks out the serial
      * bus to the current fiber.
      *
      * This method sets the required length, configures the delimeters,
      * and creates a buffer for the interrupt to populate.
      *
      * After the interrupt is configured, this method blocks the calling fiber
      */
    void setWriteInterrupt(ManagedString string);

    /**
      * An internal method to configure an interrupt on recv.
      *
      * On entry to this method, a flag is set which locks out the serial
      * bus to the current fiber.
      *
      * This method sets the required length, configures the delimeters,
      * and creates a buffer for the interrupt to populate.
      *
      * After the interrupt is configured, this method blocks the calling fiber
      */
    void setReadInterrupt(ManagedString delimeters, int len);

    /**
      * An internal method that resets the MicroBitSerial instance to an unused state after a read.
      */
    void resetRead();

    /**
      * An internal method that resets the MicroBitSerial instance to an unused state after a write.
      */
    void resetWrite();

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
      * Sends a character over serial.
      *
      * @param c the character to send
      *
      * Example:
      * @code
      * uBit.serial.send('a');
      * @endcode
      */
    int send(char c);

    /**
      * Sends a ManagedString over serial.
      *
      * @param s the ManagedString to send
      *
      * Example:
      * @code
      * uBit.serial.send("abc123");
      * @endcode
      */
    int send(ManagedString s);

    /**
      * Reads a single character from the serial bus.
      *
      * @return 0 if the serial bus is already in use, or the character read
      *         from the serial bus.
      * @note this call blocks the current fiber.
      * Example:
      * @code
      * char c = uBit.serial.read();
      * @endcode
      */
    char read();

    /**
      * Reads a single character from the serial bus.
      *
      * @param len the length of the string you would like to read from the serial bus
      * @param delimeters a series of delimeters that are evaluated on a per character
      *        basis.
      * @return MICROBIT_INVALID_PARAMETER if len is less than 2, or MICROBIT_SERIAL_IN_USE
      *         if there is a fiber currently blocking on a result from the serial bus.
      *
      * Example:
      * @code
      * ManagedString s = uBit.serial.read("",10); //reads 10 chars from the serial bus.
      * @endcode
      *
      * @note This call blocks the current fiber. The raw buffer can also be obtained from a
      *       ManagedString instance by calling toCharArray().
      */
    ManagedString read(int len = MICROBIT_SERIAL_DEFAULT_BUFFER_SIZE, ManagedString delimeters = MICROBIT_SERIAL_DEFAULT_DELIM);

    /**
      * Detaches a previously configured interrupt
      *
      * @param interruptType one of Serial::RxIrq or Serial::TxIrq
      *
      * @note #HACK, this should really be further up in the mbed libs, after
      *       attaching, you would expect to be able to detach...
      */
    void detach(Serial::IrqType interuptType);

};

#endif
