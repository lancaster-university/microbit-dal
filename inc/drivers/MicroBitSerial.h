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

#ifndef MICROBIT_SERIAL_H
#define MICROBIT_SERIAL_H

#include "mbed.h"
#include "ManagedString.h"

#define MICROBIT_SERIAL_DEFAULT_BAUD_RATE   115200
#define MICROBIT_SERIAL_DEFAULT_BUFFER_SIZE 20

#define MICROBIT_SERIAL_EVT_DELIM_MATCH     1
#define MICROBIT_SERIAL_EVT_HEAD_MATCH      2
#define MICROBIT_SERIAL_EVT_RX_FULL         3

#define MICROBIT_SERIAL_RX_IN_USE           1
#define MICROBIT_SERIAL_TX_IN_USE           2
#define MICROBIT_SERIAL_RX_BUFF_INIT        4
#define MICROBIT_SERIAL_TX_BUFF_INIT        8


enum MicroBitSerialMode
{
    ASYNC,
	SYNC_SPINWAIT,
	SYNC_SLEEP
};

/**
  * Class definition for MicroBitSerial.
  *
  * Represents an instance of RawSerial which accepts micro:bit specific data types.
  */
class MicroBitSerial : public RawSerial
{

    //holds that state of the mutex locks for all MicroBitSerial instances.
    static uint8_t status;

    //holds the state of the baudrate for all MicroBitSerial instances.
    static int baudrate;

    //delimeters used for matching on receive.
    ManagedString delimeters;

    //a variable used when a user calls the eventAfter() method.
    int rxBuffHeadMatch;

    uint8_t *rxBuff;
    uint8_t rxBuffSize;
    volatile uint16_t rxBuffHead;
    uint16_t rxBuffTail;


    uint8_t *txBuff;
    uint8_t txBuffSize;
    uint16_t txBuffHead;
    volatile uint16_t txBuffTail;

    /**
      * An internal interrupt callback for MicroBitSerial configured for when a
      * character is received.
      *
      * Each time a character is received fill our circular buffer!
      */
    void dataReceived();

    /**
      * An internal interrupt callback for MicroBitSerial.
      *
      * Each time the Serial module's buffer is empty, write a character if we have
      * characters to write.
      */
    void dataWritten();

    /**
      * An internal method to configure an interrupt on tx buffer and also
      * a best effort copy operation to move bytes from a user buffer to our txBuff
      *
      * @param string a pointer to the first character of the users' buffer.
      *
      * @param len the length of the string, and ultimately the maximum number of bytes
      *        that will be copied dependent on the state of txBuff
      *
      * @param mode determines whether to configure the current fiber context or not. If
      *             The mode is SYNC_SPINWAIT, the context will not be configured, otherwise
      *             no context will be configured.
      *
      * @return the number of bytes copied into the buffer.
      */
    int setTxInterrupt(uint8_t *string, int len, MicroBitSerialMode mode);

    /**
      * Locks the mutex so that others can't use this serial instance for reception
      */
    void lockRx();

    /**
      * Locks the mutex so that others can't use this serial instance for transmission
      */
    void lockTx();

    /**
      * Unlocks the mutex so that others can use this serial instance for reception
      */
    void unlockRx();

    /**
      * Unlocks the mutex so that others can use this serial instance for transmission
      */
    void unlockTx();

    /**
      * We do not want to always have our buffers initialised, especially if users to not
      * use them. We only bring them up on demand.
      */
    int initialiseRx();

    /**
      * We do not want to always have our buffers initialised, especially if users to not
      * use them. We only bring them up on demand.
      */
    int initialiseTx();

    /**
      * An internal method that either spin waits if mode is set to SYNC_SPINWAIT
      * or puts the fiber to sleep if the mode is set to SYNC_SLEEP
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP
      */
    void send(MicroBitSerialMode mode);

    /**
      * Reads a single character from the rxBuff
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - A character is read from the rxBuff if available, if there
      *                    are no characters to be read, a value of zero is returned immediately.
      *
      *            SYNC_SPINWAIT - A character is read from the rxBuff if available, if there
      *                            are no characters to be read, this method will spin
      *                            (lock up the processor) until a character is available.
      *
      *            SYNC_SLEEP - A character is read from the rxBuff if available, if there
      *                         are no characters to be read, the calling fiber sleeps
      *                         until there is a character available.
      *
      *         Defaults to SYNC_SLEEP.
      *
      * @return a character from the circular buffer, or MICROBIT_NO_DATA is there
      *         are no characters in the buffer.
      */
    int getChar(MicroBitSerialMode mode);

    /**
      * An internal method that copies values from a circular buffer to a linear buffer.
      *
      * @param circularBuff a pointer to the source circular buffer
      *
      * @param circularBuffSize the size of the circular buffer
      *
      * @param linearBuff a pointer to the destination linear buffer
      *
      * @param tailPosition the tail position in the circular buffer you want to copy from
      *
      * @param headPosition the head position in the circular buffer you want to copy to
      *
      * @note this method assumes that the linear buffer has the appropriate amount of
      *       memory to contain the copy operation
      */
    void circularCopy(uint8_t *circularBuff, uint8_t circularBuffSize, uint8_t *linearBuff, uint16_t tailPosition, uint16_t headPosition);

    public:

    /**
      * Constructor.
      * Create an instance of MicroBitSerial
      *
      * @param tx the Pin to be used for transmission
      *
      * @param rx the Pin to be used for receiving data
      *
      * @param rxBufferSize the size of the buffer to be used for receiving bytes
      *
      * @param txBufferSize the size of the buffer to be used for transmitting bytes
      *
      * @code
      * MicroBitSerial serial(USBTX, USBRX);
      * @endcode
      * @note the default baud rate is 115200. More API details can be found:
      *       -https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/api/SerialBase.h
      *       -https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/api/RawSerial.h
      *
      *       Buffers aren't allocated until the first send or receive respectively.
      */
    MicroBitSerial(PinName tx, PinName rx, uint8_t rxBufferSize = MICROBIT_SERIAL_DEFAULT_BUFFER_SIZE, uint8_t txBufferSize = MICROBIT_SERIAL_DEFAULT_BUFFER_SIZE);

    /**
      * Sends a single character over the serial line.
      *
      * @param c the character to send
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - the character is copied into the txBuff and returns immediately.
      *
      *            SYNC_SPINWAIT - the character is copied into the txBuff and this method
      *                            will spin (lock up the processor) until the character has
      *                            been sent.
      *
      *            SYNC_SLEEP - the character is copied into the txBuff and the fiber sleeps
      *                         until the character has been sent. This allows other fibers
      *                         to continue execution.
      *
      *         Defaults to SYNC_SLEEP.
      *
      * @return the number of bytes written, or MICROBIT_SERIAL_IN_USE if another fiber
      *         is using the serial instance for transmission.
      */
    int sendChar(char c, MicroBitSerialMode mode = MICROBIT_DEFAULT_SERIAL_MODE);

    /**
      * Sends a ManagedString over the serial line.
      *
      * @param s the string to send
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - bytes are copied into the txBuff and returns immediately.
      *
      *            SYNC_SPINWAIT - bytes are copied into the txBuff and this method
      *                            will spin (lock up the processor) until all bytes
      *                            have been sent.
      *
      *            SYNC_SLEEP - bytes are copied into the txBuff and the fiber sleeps
      *                         until all bytes have been sent. This allows other fibers
      *                         to continue execution.
      *
      *         Defaults to SYNC_SLEEP.
      *
      * @return the number of bytes written, MICROBIT_SERIAL_IN_USE if another fiber
      *         is using the serial instance for transmission, MICROBIT_INVALID_PARAMETER
      *         if buffer is invalid, or the given bufferLen is <= 0.
      */
    int send(ManagedString s, MicroBitSerialMode mode = MICROBIT_DEFAULT_SERIAL_MODE);

    /**
      * Sends a buffer of known length over the serial line.
      *
      * @param buffer a pointer to the first character of the buffer
      *
      * @param len the number of bytes that are safely available to read.
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - bytes are copied into the txBuff and returns immediately.
      *
      *            SYNC_SPINWAIT - bytes are copied into the txBuff and this method
      *                            will spin (lock up the processor) until all bytes
      *                            have been sent.
      *
      *            SYNC_SLEEP - bytes are copied into the txBuff and the fiber sleeps
      *                         until all bytes have been sent. This allows other fibers
      *                         to continue execution.
      *
      *         Defaults to SYNC_SLEEP.
      *
      * @return the number of bytes written, MICROBIT_SERIAL_IN_USE if another fiber
      *         is using the serial instance for transmission, MICROBIT_INVALID_PARAMETER
      *         if buffer is invalid, or the given bufferLen is <= 0.
      */
    int send(uint8_t *buffer, int bufferLen, MicroBitSerialMode mode = MICROBIT_DEFAULT_SERIAL_MODE);

    /**
      * Reads a single character from the rxBuff
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - A character is read from the rxBuff if available, if there
      *                    are no characters to be read, a value of MICROBIT_NO_DATA is returned immediately.
      *
      *            SYNC_SPINWAIT - A character is read from the rxBuff if available, if there
      *                            are no characters to be read, this method will spin
      *                            (lock up the processor) until a character is available.
      *
      *            SYNC_SLEEP - A character is read from the rxBuff if available, if there
      *                         are no characters to be read, the calling fiber sleeps
      *                         until there is a character available.
      *
      *         Defaults to SYNC_SLEEP.
      *
      * @return a character, MICROBIT_SERIAL_IN_USE if another fiber is using the serial instance for reception,
      *         MICROBIT_NO_RESOURCES if buffer allocation did not complete successfully, or MICROBIT_NO_DATA if
      *         the rx buffer is empty and the mode given is ASYNC.
      */
    int read(MicroBitSerialMode mode = MICROBIT_DEFAULT_SERIAL_MODE);

    /**
      * Reads multiple characters from the rxBuff and returns them as a ManagedString
      *
      * @param size the number of characters to read.
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - If the desired number of characters are available, this will return
      *                    a ManagedString with the expected size. Otherwise, it will read however
      *                    many characters there are available.
      *
      *            SYNC_SPINWAIT - If the desired number of characters are available, this will return
      *                            a ManagedString with the expected size. Otherwise, this method will spin
      *                            (lock up the processor) until the desired number of characters have been read.
      *
      *            SYNC_SLEEP - If the desired number of characters are available, this will return
      *                         a ManagedString with the expected size. Otherwise, the calling fiber sleeps
      *                         until the desired number of characters have been read.
      *
      *         Defaults to SYNC_SLEEP.
      *
      * @return A ManagedString, or an empty ManagedString if an error was encountered during the read.
      */
    ManagedString read(int size, MicroBitSerialMode mode = MICROBIT_DEFAULT_SERIAL_MODE);

    /**
      * Reads multiple characters from the rxBuff and fills a user buffer.
      *
      * @param buffer a pointer to a user allocated buffer.
      *
      * @param bufferLen the amount of data that can be safely stored
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - If the desired number of characters are available, this will fill
      *                    the given buffer. Otherwise, it will fill the buffer with however
      *                    many characters there are available.
      *
      *            SYNC_SPINWAIT - If the desired number of characters are available, this will fill
      *                            the given buffer. Otherwise, this method will spin (lock up the processor)
      *                            and fill the buffer until the desired number of characters have been read.
      *
      *            SYNC_SLEEP - If the desired number of characters are available, this will fill
      *                         the given buffer. Otherwise, the calling fiber sleeps
      *                         until the desired number of characters have been read.
      *
      *         Defaults to SYNC_SLEEP.
      *
      * @return the number of characters read, or MICROBIT_SERIAL_IN_USE if another fiber
      *         is using the instance for receiving.
      */
    int read(uint8_t *buffer, int bufferLen, MicroBitSerialMode mode = MICROBIT_DEFAULT_SERIAL_MODE);

    /**
      * Reads until one of the delimeters matches a character in the rxBuff
      *
      * @param delimeters a ManagedString containing a sequence of delimeter characters e.g. ManagedString("\r\n")
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - If one of the delimeters matches a character already in the rxBuff
      *                    this method will return a ManagedString up to the delimeter.
      *                    Otherwise, it will return an Empty ManagedString.
      *
      *            SYNC_SPINWAIT - If one of the delimeters matches a character already in the rxBuff
      *                            this method will return a ManagedString up to the delimeter.
      *                            Otherwise, this method will spin (lock up the processor) until a
      *                            received character matches one of the delimeters.
      *
      *            SYNC_SLEEP - If one of the delimeters matches a character already in the rxBuff
      *                         this method will return a ManagedString up to the delimeter.
      *                         Otherwise, the calling fiber sleeps until a character matching one
      *                         of the delimeters is seen.
      *
      *         Defaults to SYNC_SLEEP.
      *
      * @return A ManagedString containing the characters up to a delimeter, or an Empty ManagedString,
      *         if another fiber is currently using this instance for reception.
      *
      * @note delimeters are matched on a per byte basis.
      */
    ManagedString readUntil(ManagedString delimeters, MicroBitSerialMode mode = MICROBIT_DEFAULT_SERIAL_MODE);

    /**
      * A wrapper around the inherited method "baud" so we can trap the baud rate
      * as it changes and restore it if redirect() is called.
      *
      * @param baudrate the new baudrate. See:
      *         - https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/targets/hal/TARGET_NORDIC/TARGET_MCU_NRF51822/serial_api.c
      *        for permitted baud rates.
      *
      * @return MICROBIT_INVALID_PARAMETER if baud rate is less than 0, otherwise MICROBIT_OK.
      *
      * @note the underlying implementation chooses the first allowable rate at or above that requested.
      */
    void baud(int baudrate);

    /**
      * A way of dynamically configuring the serial instance to use pins other than USBTX and USBRX.
      *
      * @param tx the new transmission pin.
      *
      * @param rx the new reception pin.
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently transmitting or receiving, otherwise MICROBIT_OK.
      */
    int redirect(PinName tx, PinName rx);

    /**
      * Configures an event to be fired after "len" characters.
      *
      * Will generate an event with the ID: MICROBIT_ID_SERIAL and the value MICROBIT_SERIAL_EVT_HEAD_MATCH.
      *
      * @param len the number of characters to wait before triggering the event.
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - Will configure the event and return immediately.
      *
      *            SYNC_SPINWAIT - will return MICROBIT_INVALID_PARAMETER
      *
      *            SYNC_SLEEP - Will configure the event and block the current fiber until the
      *                         event is received.
      *
      * @return MICROBIT_INVALID_PARAMETER if the mode given is SYNC_SPINWAIT, otherwise MICROBIT_OK.
      */
    int eventAfter(int len, MicroBitSerialMode mode = ASYNC);

    /**
      * Configures an event to be fired on a match with one of the delimeters.
      *
      * Will generate an event with the ID: MICROBIT_ID_SERIAL and the value MICROBIT_SERIAL_EVT_DELIM_MATCH.
      *
      * @param delimeters the characters to match received characters against e.g. ManagedString("\n")
      *
      * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP. Each mode
      *        gives a different behaviour:
      *
      *            ASYNC - Will configure the event and return immediately.
      *
      *            SYNC_SPINWAIT - will return MICROBIT_INVALID_PARAMETER
      *
      *            SYNC_SLEEP - Will configure the event and block the current fiber until the
      *                         event is received.
      *
      * @return MICROBIT_INVALID_PARAMETER if the mode given is SYNC_SPINWAIT, otherwise MICROBIT_OK.
      *
      * @note delimeters are matched on a per byte basis.
      */
    int eventOn(ManagedString delimeters, MicroBitSerialMode mode = ASYNC);

    /**
      * Determines whether there is any data waiting in our Rx buffer.
      *
      * @return 1 if we have space, 0 if we do not.
      *
      * @note We do not wrap the super's readable() method as we don't want to
      *       interfere with communities that use manual calls to serial.readable().
      */
    int isReadable();

    /**
      * Determines if we have space in our txBuff.
      *
      * @return 1 if we have space, 0 if we do not.
      *
      * @note We do not wrap the super's writeable() method as we don't want to
      *       interfere with communities that use manual calls to serial.writeable().
      */
    int isWriteable();

    /**
      * Reconfigures the size of our rxBuff
      *
      * @param size the new size for our rxBuff
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
      *         for reception, otherwise MICROBIT_OK.
      */
    int setRxBufferSize(uint8_t size);

    /**
      * Reconfigures the size of our txBuff
      *
      * @param size the new size for our txBuff
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
      *         for transmission, otherwise MICROBIT_OK.
      */
    int setTxBufferSize(uint8_t size);

    /**
      * The size of our rx buffer in bytes.
      *
      * @return the current size of rxBuff in bytes
      */
    int getRxBufferSize();

    /**
      * The size of our tx buffer in bytes.
      *
      * @return the current size of txBuff in bytes
      */
    int getTxBufferSize();

    /**
      * Sets the tail to match the head of our circular buffer for reception,
      * effectively clearing the reception buffer.
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
      *         for reception, otherwise MICROBIT_OK.
      */
    int clearRxBuffer();

    /**
      * Sets the tail to match the head of our circular buffer for transmission,
      * effectively clearing the transmission buffer.
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
      *         for transmission, otherwise MICROBIT_OK.
      */
    int clearTxBuffer();

    /**
      * The number of bytes currently stored in our rx buffer waiting to be digested,
      * by the user.
      *
      * @return The currently buffered number of bytes in our rxBuff.
      */
    int rxBufferedSize();

    /**
      * The number of bytes currently stored in our tx buffer waiting to be transmitted
      * by the hardware.
      *
      * @return The currently buffered number of bytes in our txBuff.
      */
    int txBufferedSize();

    /**
      * Determines if the serial bus is currently in use by another fiber for reception.
      *
      * @return The state of our mutex lock for reception.
      *
      * @note Only one fiber can call read at a time
      */
    int rxInUse();

    /**
      * Determines if the serial bus is currently in use by another fiber for transmission.
      *
      * @return The state of our mutex lock for transmition.
      *
      * @note Only one fiber can call send at a time
      */
    int txInUse();

    /**
      * Detaches a previously configured interrupt
      *
      * @param interruptType one of Serial::RxIrq or Serial::TxIrq
      */
    void detach(Serial::IrqType interuptType);

};

#endif
