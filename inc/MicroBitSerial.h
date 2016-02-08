#ifndef MICROBIT_SERIAL_H
#define MICROBIT_SERIAL_H

#include "mbed.h"
#include "ManagedString.h"
#include "MicroBitImage.h"

#define MICROBIT_SERIAL_DEFAULT_BAUD_RATE   115200
#define MICROBIT_SERIAL_DEFAULT_BUFFER_SIZE 20

#define MICROBIT_SERIAL_EVT_TX_EMPTY        1
#define MICROBIT_SERIAL_EVT_DELIM_MATCH     2
#define MICROBIT_SERIAL_EVT_HEAD_MATCH      3
#define MICROBIT_SERIAL_EVT_RX_FULL         4

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
  * Represents an instance of Serial which accepts micro:bit specific data types.
  */
class MicroBitSerial : public Serial
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
      * @param string a pointer to the first character of the users' buffer
      * @param len the length of the string, and ultimately the maximum number of bytes
      *        that will be copied dependent on the state of txBuff
      *
      * @return the number of bytes copied into the buffer.
      */
    int setTxInterrupt(uint8_t *string, int len);

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
      * An internal method that retreives the most recent character from the rxBuff.
      * It updates the rxBuffTail also.
      *
      * @return the most character, or 0 if there are no characters available.
      */
    char getChar(MicroBitSerialMode mode);

    /**
      * An internal method that copies values from a circular buffer to a linear buffer.
      *
      * @param circularBuff a pointer to the source circular buffer
      * @param circularBuffSize the size of the circular buffer
      * @param linearBuff a pointer to the destination linear buffer
      * @param tailPosition the tail position in the circular buffer you want to copy from
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
      * @param rx the Pin to be used for receiving data
      * @param rxBufferSize the size of the buffer to be used for receiving bytes
      * @param txBufferSize the size of the buffer to be used for transmitting bytes
      *
      * Example:
      * @code
      * MicroBitSerial serial(USBTX, USBRX);
      * @endcode
      * @note the default baud rate is 115200. More API details can be found:
      *       -https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/api/SerialBase.h
      *       -https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/api/Serial.h
      *
      *       Buffers aren't allocated until the first send or receive respectively.
      */
    MicroBitSerial(PinName tx, PinName rx, uint8_t rxBufferSize = MICROBIT_SERIAL_DEFAULT_BUFFER_SIZE, uint8_t txBufferSize = MICROBIT_SERIAL_DEFAULT_BUFFER_SIZE);

    /**
      * Sends a single character via Serial
      *
      * @param c the character to send
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
    int send(char c, MicroBitSerialMode mode = SYNC_SLEEP);

    /**
      * Sends a ManagedString via Serial
      *
      * @param s the string to send
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
      * @return the number of bytes written, or MICROBIT_SERIAL_IN_USE if another fiber
      *         is using the serial instance for transmission.
      */
    int send(ManagedString s, MicroBitSerialMode mode = SYNC_SLEEP);

    /**
      * Sends a ManagedString via Serial
      *
      * @param buffer a pointer to the first character of the buffer
      * @param len the number of bytes that are safely available to read.
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
      * @return the number of bytes written, or MICROBIT_SERIAL_IN_USE if another fiber
      *         is using the serial instance for transmission.
      */
    int send(uint8_t *buffer, int bufferLen, MicroBitSerialMode mode = SYNC_SLEEP);

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
      * @return a character, or 0 if another fiber is using the serial instance for reception.
      */
    char read(MicroBitSerialMode mode = SYNC_SLEEP);

    /**
      * Reads multiple characters from the rxBuff and returns them as a ManagedString
      *
      * @param size the number of characters to read.
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
      * @return A ManagedString with the length > 0, or an empty ManagedString if
      *         another fiber is using the MicroBitSerial instance.
      */
    ManagedString read(int size, MicroBitSerialMode mode = SYNC_SLEEP);

    /**
      * Reads multiple characters from the rxBuff and fills a user buffer.
      *
      * @param buffer a pointer to a user allocated buffer
      * @param bufferLen the amount of data that can be safely stored
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
    int read(uint8_t *buffer, int bufferLen, MicroBitSerialMode mode = SYNC_SLEEP);

    /**
      * Reads until one of the delimeters matches a character in the rxBuff
      *
      * @param delimeters a ManagedString containing a sequence of delimeter characters e.g. ManagedString("\r\n")
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
      */
    ManagedString readUntil(ManagedString delimeters, MicroBitSerialMode mode = SYNC_SLEEP);

    /**
      * A wrapper around the inherited method "baud" so we can trap the baud rate
      * as it changes and restore it if redirect() is called.
      * @param baudrate the new baudrate. See:
      *         - https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/targets/hal/TARGET_NORDIC/TARGET_MCU_NRF51822/serial_api.c
      *        for permitted baud rates.
      *
      * @return MICROBIT_INVALID_PARAMETER if baud rate is less than 0, otherwise MICROBIT_OK.
      */
    void baud(int baudrate);

    /**
      * A way of dynamically configuring the serial instance to use pins other than USBTX and USBRX.
      *
      * @param tx the new transmission pin.
      * @param rx the new reception pin.
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently transmitting or receiving, otherwise MICROBIT_OK.
      */
    int redirect(PinName tx, PinName rx);

    /**
      * Configures an event to be fired after "len" characters.
      *
      * @param len the number of characters to wait before triggering the event
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
      * @param delims the characters to match received characters against e.g. ManagedString("\r\n")
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
    int eventOn(ManagedString delims, MicroBitSerialMode mode = ASYNC);

    /**
      * Determines if we have space in our rxBuff.
      *
      * @return 1 if we have space, 0 if we do not.
      *
      * @note the reason we do not wrap the super's readable() method is so that we
      *       don't interfere with communities that use manual calls to uBit.serial.readable()
      */
    int isReadable();

    /**
      * Determines if we have space in our txBuff.
      *
      * @return 1 if we have space, 0 if we do not.
      *
      * @note the reason we do not wrap the super's write() method is so that we
      *       don't interfere with communities that use manual calls to uBit.serial.writeable()
      */
    int isWriteable();

    /**
      * reconfigures the size of our rxBuff
      *
      * @param size the new size for our rxBuff
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
      *         for reception, otherwise MICROBIT_OK.
      */
    int setRxBufferSize(uint8_t size);

    /**
      * reconfigures the size of our txBuff
      *
      * @param size the new size for our txBuff
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
      *         for transmission, otherwise MICROBIT_OK.
      */
    int setTxBufferSize(uint8_t size);

    /**
      * @return the current size of our rxBuff in bytes
      */
    int getRxBufferSize();

    /**
      * @return the current size of our txBuff in bytes
      */
    int getTxBufferSize();

    /**
      * Sets the tail to match the head of our circular buffer for reception
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
      *         for reception, otherwise MICROBIT_OK.
      */
    int clearRxBuffer();

    /**
      * Sets the tail to match the head of our circular buffer for transmission
      *
      * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
      *         for transmission, otherwise MICROBIT_OK.
      */
    int clearTxBuffer();

    /**
      * @return The currently buffered number of bytes in our rxBuff.
      */
    int rxBufferedSize();

    /**
      * @return The currently buffered number of bytes in our txBuff.
      */
    int txBufferedSize();

    /**
      * @return The state of our mutex lock for reception.
      *
      * @note only one fiber can call read at a time
      */
    int rxInUse();

    /**
      * @return The state of our mutex lock for transmition.
      *
      * @note only one fiber can call send at a time
      */
    int txInUse();

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
