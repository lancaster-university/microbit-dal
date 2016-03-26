#include "mbed.h"
#include "MicroBitSerial.h"
#include "ErrorNo.h"
#include "MicroBitComponent.h"
#include "MicroBitFiber.h"
#include "NotifyEvents.h"

uint8_t MicroBitSerial::status = 0;

int MicroBitSerial::baudrate = 0;

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
MicroBitSerial::MicroBitSerial(PinName tx, PinName rx, uint8_t rxBufferSize, uint8_t txBufferSize) : RawSerial(tx,rx), delimeters()
{
    this->rxBuffSize = rxBufferSize;
    this->txBuffSize = txBufferSize;

    this->rxBuff = NULL;
    this->txBuff = NULL;

    this->rxBuffHead = 0;
    this->rxBuffTail = 0;

    this->txBuffHead = 0;
    this->txBuffTail = 0;

    this->rxBuffHeadMatch = -1;

    this->baud(MICROBIT_SERIAL_DEFAULT_BAUD_RATE);
}

/**
  * An internal interrupt callback for MicroBitSerial configured for when a
  * character is received.
  *
  * Each time a character is received fill our circular buffer!
  */
void MicroBitSerial::dataReceived()
{
    if(!(status & MICROBIT_SERIAL_RX_BUFF_INIT))
        return;

    //get the received character
    char c = getc();

    int delimeterOffset = 0;
    int delimLength = this->delimeters.length();

    //iterate through our delimeters (if any) to see if there is a match
    while(delimeterOffset < delimLength)
    {
        //fire an event if there is to block any waiting fibers
        if(this->delimeters.charAt(delimeterOffset) == c)
            MicroBitEvent(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_DELIM_MATCH);

        delimeterOffset++;
    }

    uint16_t newHead = (rxBuffHead + 1) % rxBuffSize;

    //look ahead to our newHead value to see if we are about to collide with the tail
    if(newHead != rxBuffTail)
    {
        //if we are not, store the character, and update our actual head.
        this->rxBuff[rxBuffHead] = c;
        rxBuffHead = newHead;

        //if we have any fibers waiting for a specific number of characters, unblock them
        if(rxBuffHeadMatch >= 0)
            if(rxBuffHead == rxBuffHeadMatch)
            {
                rxBuffHeadMatch = -1;
                MicroBitEvent(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_HEAD_MATCH);
            }
    }
    else
        //otherwise, our buffer is full, send an event to the user...
        MicroBitEvent(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_RX_FULL);
}

/**
  * An internal interrupt callback for MicroBitSerial.
  *
  * Each time the Serial module's buffer is empty, write a character if we have
  * characters to write.
  */
void MicroBitSerial::dataWritten()
{
    if(txBuffTail == txBuffHead || !(status & MICROBIT_SERIAL_TX_BUFF_INIT))
        return;

    //send our current char
    putc(txBuff[txBuffTail]);

    uint16_t nextTail = (txBuffTail + 1) % txBuffSize;

    //unblock any waiting fibers that are waiting for transmission to finish.
    if(nextTail == txBuffHead)
    {
        MicroBitEvent(MICROBIT_ID_NOTIFY, MICROBIT_SERIAL_EVT_TX_EMPTY);
        detach(Serial::IrqType::TxIrq);
    }

    //update our tail!
    txBuffTail = nextTail;
}

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
int MicroBitSerial::setTxInterrupt(uint8_t *string, int len)
{
    int copiedBytes = 0;

    for(copiedBytes = 0; copiedBytes < len; copiedBytes++)
    {
        uint16_t nextHead = (txBuffHead + 1) % txBuffSize;
        if(nextHead != txBuffTail)
        {
            this->txBuff[txBuffHead] = string[copiedBytes];
            txBuffHead = nextHead;
        }
        else
            break;
    }

    fiber_wake_on_event(MICROBIT_ID_NOTIFY, MICROBIT_SERIAL_EVT_TX_EMPTY);

    //set the TX interrupt
    attach(this, &MicroBitSerial::dataWritten, Serial::IrqType::TxIrq);

    return copiedBytes;
}

/**
  * Locks the mutex so that others can't use this serial instance for reception
  */
void MicroBitSerial::lockRx()
{
    status |= MICROBIT_SERIAL_RX_IN_USE;
}

/**
  * Locks the mutex so that others can't use this serial instance for transmission
  */
void MicroBitSerial::lockTx()
{
    status |= MICROBIT_SERIAL_TX_IN_USE;
}

/**
  * Unlocks the mutex so that others can use this serial instance for reception
  */
void MicroBitSerial::unlockRx()
{
    status &= ~MICROBIT_SERIAL_RX_IN_USE;
}

/**
  * Unlocks the mutex so that others can use this serial instance for transmission
  */
void MicroBitSerial::unlockTx()
{
    status &= ~MICROBIT_SERIAL_TX_IN_USE;
}

/**
  * We do not want to always have our buffers initialised, especially if users to not
  * use them. We only bring them up on demand.
  */
int MicroBitSerial::initialiseRx()
{
    if((status & MICROBIT_SERIAL_RX_BUFF_INIT))
    {
        //ensure that we receive no interrupts after freeing our buffer
        detach(Serial::IrqType::RxIrq);
        free(this->rxBuff);
    }

    status &= ~MICROBIT_SERIAL_RX_BUFF_INIT;

    if((this->rxBuff = (uint8_t *)malloc(rxBuffSize)) == NULL)
        return MICROBIT_NO_RESOURCES;

    this->rxBuffHead = 0;
    this->rxBuffTail = 0;

    //set the receive interrupt
    status |= MICROBIT_SERIAL_RX_BUFF_INIT;
    attach(this, &MicroBitSerial::dataReceived, Serial::IrqType::RxIrq);

    return MICROBIT_OK;
}

/**
  * We do not want to always have our buffers initialised, especially if users to not
  * use them. We only bring them up on demand.
  */
int MicroBitSerial::initialiseTx()
{
    if((status & MICROBIT_SERIAL_TX_BUFF_INIT))
    {
        //ensure that we receive no interrupts after freeing our buffer
        detach(Serial::IrqType::TxIrq);
        free(this->txBuff);
    }

    status &= ~MICROBIT_SERIAL_TX_BUFF_INIT;

    if((this->txBuff = (uint8_t *)malloc(txBuffSize)) == NULL)
        return MICROBIT_NO_RESOURCES;

    this->txBuffHead = 0;
    this->txBuffTail = 0;

    status |= MICROBIT_SERIAL_TX_BUFF_INIT;

    return MICROBIT_OK;
}

/**
  * An internal method that either spin waits if mode is set to SYNC_SPINWAIT
  * or puts the fiber to sleep if the mode is set to SYNC_SLEEP
  *
  * @param mode the selected mode, one of: ASYNC, SYNC_SPINWAIT, SYNC_SLEEP
  */
void MicroBitSerial::send(MicroBitSerialMode mode)
{
    if(mode == SYNC_SPINWAIT)
        while(txBufferedSize() > 0);

    if(mode == SYNC_SLEEP)
        fiber_sleep(0);
}

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
  * @return the a character from the circular buffer, or MICROBIT_NO_DATA is there a no characters in the buffer.
  */
int MicroBitSerial::getChar(MicroBitSerialMode mode)
{
    if(mode == ASYNC)
    {
        if(!isReadable())
            return MICROBIT_NO_DATA;
    }

    if(mode == SYNC_SPINWAIT)
        while(!isReadable());

    if(mode == SYNC_SLEEP)
    {
        if(!isReadable())
            eventAfter(1, mode);
    }

    char c = rxBuff[rxBuffTail];

    rxBuffTail = (rxBuffTail + 1) % rxBuffSize;

    return c;
}

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
void MicroBitSerial::circularCopy(uint8_t *circularBuff, uint8_t circularBuffSize, uint8_t *linearBuff, uint16_t tailPosition, uint16_t headPosition)
{
    int toBuffIndex = 0;

    while(tailPosition != headPosition)
    {
        linearBuff[toBuffIndex++] = circularBuff[tailPosition];

        tailPosition = (tailPosition + 1) % circularBuffSize;
    }
}

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
int MicroBitSerial::sendChar(char c, MicroBitSerialMode mode)
{
    if(txInUse())
        return MICROBIT_SERIAL_IN_USE;

    lockTx();

    //lazy initialisation of our tx buffer
    if(!(status & MICROBIT_SERIAL_TX_BUFF_INIT))
    {
        int result = initialiseTx();

        if(result != MICROBIT_OK)
            return result;
    }

    uint8_t toTransmit[2] =  { c, '\0'};

    int bytesWritten = setTxInterrupt(toTransmit, 1);

    send(mode);

    unlockTx();

    return bytesWritten;
}

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
int MicroBitSerial::send(ManagedString s, MicroBitSerialMode mode)
{
    return send((uint8_t *)s.toCharArray(), s.length(), mode);
}

/**
  * Sends a buffer of known length via serial
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
int MicroBitSerial::send(uint8_t *buffer, int bufferLen, MicroBitSerialMode mode)
{
    if(txInUse())
        return MICROBIT_SERIAL_IN_USE;

    lockTx();

    //lazy initialisation of our tx buffer
    if(!(status & MICROBIT_SERIAL_TX_BUFF_INIT))
    {
        int result = initialiseTx();

        if(result != MICROBIT_OK)
            return result;
    }

    int bytesWritten = setTxInterrupt(buffer, bufferLen);

    send(mode);

    unlockTx();

    return bytesWritten;
}

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
int MicroBitSerial::read(MicroBitSerialMode mode)
{
    if(rxInUse())
        return MICROBIT_SERIAL_IN_USE;

    lockRx();

    //lazy initialisation of our buffers
    if(!(status & MICROBIT_SERIAL_RX_BUFF_INIT))
    {
        int result = initialiseRx();

        if(result != MICROBIT_OK)
            return result;
    }

    char c = (char)getChar(mode);

    unlockRx();

    return c;
}

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
  *         an error was encountered during the read.
  */
ManagedString MicroBitSerial::read(int size, MicroBitSerialMode mode)
{
    uint8_t buff[size + 1] = { 0 };

    int returnedSize = read((uint8_t *)buff, size, mode);

    if(returnedSize <= 0)
        return ManagedString();

    return ManagedString((char *)buff, returnedSize);
}

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
int MicroBitSerial::read(uint8_t *buffer, int bufferLen, MicroBitSerialMode mode)
{
    if(rxInUse())
        return MICROBIT_SERIAL_IN_USE;

    lockRx();

    //lazy initialisation of our rx buffer
    if(!(status & MICROBIT_SERIAL_RX_BUFF_INIT))
    {
        int result = initialiseRx();

        if(result != MICROBIT_OK)
            return result;
    }

    int bufferIndex = 0;

    int temp = 0;

    if(mode == ASYNC)
    {
        while((temp = getChar(mode)) != MICROBIT_NO_DATA && bufferIndex < bufferLen)
        {
            buffer[bufferIndex] = (char)temp;
            bufferIndex++;
        }
    }

    if(mode == SYNC_SPINWAIT)
    {
        while(bufferIndex < bufferLen)
        {
            buffer[bufferIndex] = (char)getChar(mode);
            bufferIndex++;
        }
    }

    if(mode == SYNC_SLEEP)
    {
        if(bufferLen > rxBufferedSize())
            eventAfter(bufferLen - rxBufferedSize(), mode);

        while(bufferIndex < bufferLen)
        {
            buffer[bufferIndex] = (char)getChar(mode);
            bufferIndex++;
        }
    }

    unlockRx();

    return bufferIndex;
}


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
  *
  * @note delimeters are matched on a per byte basis.
  */
ManagedString MicroBitSerial::readUntil(ManagedString delimeters, MicroBitSerialMode mode)
{

    if(rxInUse())
        return ManagedString();

    //lazy initialisation of our rx buffer
    if(!(status & MICROBIT_SERIAL_RX_BUFF_INIT))
    {
        int result = initialiseRx();

        if(result != MICROBIT_OK)
            return result;
    }

    lockRx();

    int localTail = rxBuffTail;
    int preservedTail = rxBuffTail;

    int foundIndex = -1;

    //ASYNC mode just iterates through our stored characters checking for any matches.
    while(localTail != rxBuffHead && foundIndex  == -1)
    {
        //we use localTail to prevent modification of the actual tail.
        char c = rxBuff[localTail];

        for(int delimeterIterator = 0; delimeterIterator < delimeters.length(); delimeterIterator++)
            if(delimeters.charAt(delimeterIterator) == c)
                foundIndex = localTail;

        localTail = (localTail + 1) % rxBuffSize;
    }

    //if our mode is SYNC_SPINWAIT and we didn't see any matching characters in our buffer
    //spin until we find a match!
    if(mode == SYNC_SPINWAIT)
    {
        while(foundIndex == -1)
        {
            while(localTail == rxBuffHead);

            char c = rxBuff[localTail];

            for(int delimeterIterator = 0; delimeterIterator < delimeters.length(); delimeterIterator++)
                if(delimeters.charAt(delimeterIterator) == c)
                    foundIndex = localTail;

            localTail = (localTail + 1) % rxBuffSize;
        }
    }

    //if our mode is SYNC_SLEEP, we set up an event to be fired when we see a
    //matching character.
    if(mode == SYNC_SLEEP && foundIndex == -1)
    {
        eventOn(delimeters, mode);

        foundIndex = rxBuffHead - 1;

        this->delimeters = ManagedString();
    }

    if(foundIndex >= 0)
    {
        //calculate our local buffer size
        int localBuffSize = (preservedTail > foundIndex) ? (rxBuffSize - preservedTail) + foundIndex : foundIndex - preservedTail;

        uint8_t localBuff[localBuffSize + 1] = { 0 };

        circularCopy(rxBuff, rxBuffSize, localBuff, preservedTail, foundIndex);

        //plus one for the character we listened for...
        rxBuffTail = (rxBuffTail + localBuffSize + 1) % rxBuffSize;

        unlockRx();

        return ManagedString((char *)localBuff, localBuffSize);
    }

    unlockRx();

    return ManagedString();
}

/**
  * A wrapper around the inherited method "baud" so we can trap the baud rate
  * as it changes and restore it if redirect() is called.
  * @param baudrate the new baudrate. See:
  *         - https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/targets/hal/TARGET_NORDIC/TARGET_MCU_NRF51822/serial_api.c
  *        for permitted baud rates.
  *
  * @return MICROBIT_INVALID_PARAMETER if baud rate is less than 0, otherwise MICROBIT_OK.
  *
  * @note the underlying implementation chooses the first allowable rate at or above that requested.
  */
void MicroBitSerial::baud(int baudrate)
{
    if(baudrate < 0)
        return;

    this->baudrate = baudrate;

    RawSerial::baud(baudrate);
}

/**
  * A way of dynamically configuring the serial instance to use pins other than USBTX and USBRX.
  *
  * @param tx the new transmission pin.
  * @param rx the new reception pin.
  *
  * @return MICROBIT_SERIAL_IN_USE if another fiber is currently transmitting or receiving, otherwise MICROBIT_OK.
  */
int MicroBitSerial::redirect(PinName tx, PinName rx)
{
    if(txInUse() || rxInUse())
        return MICROBIT_SERIAL_IN_USE;

    lockTx();
    lockRx();

    if(txBufferedSize() > 0)
        detach(Serial::IrqType::TxIrq);

    detach(Serial::IrqType::RxIrq);

    serial_free(&_serial);
    serial_init(&_serial, tx, rx);

    attach(this, &MicroBitSerial::dataReceived, Serial::IrqType::RxIrq);

    if(txBufferedSize() > 0)
        attach(this, &MicroBitSerial::dataWritten, Serial::IrqType::TxIrq);

    this->baud(this->baudrate);

    unlockRx();
    unlockTx();

    return MICROBIT_OK;
}

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
int MicroBitSerial::eventAfter(int len, MicroBitSerialMode mode)
{
    if(mode == SYNC_SPINWAIT)
        return MICROBIT_INVALID_PARAMETER;

    //configure our head match...
    this->rxBuffHeadMatch = (rxBuffHead + len) % rxBuffSize;

    //block!
    if(mode == SYNC_SLEEP)
        fiber_wait_for_event(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_HEAD_MATCH);

    return MICROBIT_OK;
}

/**
  * Configures an event to be fired on a match with one of the delimeters.
  *
  * @param delimeters the characters to match received characters against e.g. ManagedString("\r\n")
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
int MicroBitSerial::eventOn(ManagedString delimeters, MicroBitSerialMode mode)
{
    if(mode == SYNC_SPINWAIT)
        return MICROBIT_INVALID_PARAMETER;

    //configure our head match...
    this->delimeters = delimeters;

    //block!
    if(mode == SYNC_SLEEP)
        fiber_wait_for_event(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_DELIM_MATCH);

    return MICROBIT_OK;
}

/**
  * Determines if we have space in our rxBuff.
  *
  * @return 1 if we have space, 0 if we do not.
  *
  * @note the reason we do not wrap the super's readable() method is so that we
  *       don't interfere with communities that use manual calls to uBit.serial.readable()
  */
int MicroBitSerial::isReadable()
{
    return (rxBuffTail != rxBuffHead) ? 1 : 0;
}

/**
  * Determines if we have space in our txBuff.
  *
  * @return 1 if we have space, 0 if we do not.
  *
  * @note the reason we do not wrap the super's write() method is so that we
  *       don't interfere with communities that use manual calls to uBit.serial.writeable()
  */
int MicroBitSerial::isWriteable()
{
    return (txBuffHead != (txBuffTail - 1)) ? 1 : 0;
}

/**
  * reconfigures the size of our rxBuff
  *
  * @param size the new size for our rxBuff
  *
  * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
  *         for reception, otherwise MICROBIT_OK or MICROBIT_NO_RESOURCES if malloc of the buffer fails.
  */
int MicroBitSerial::setRxBufferSize(uint8_t size)
{
    if(rxInUse())
        return MICROBIT_SERIAL_IN_USE;

    lockRx();

    this->rxBuffSize = size;

    int result = initialiseRx();

    unlockRx();

    return result;
}

/**
  * reconfigures the size of our txBuff
  *
  * @param size the new size for our txBuff
  *
  * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
  *         for transmission, otherwise MICROBIT_OK or MICROBIT_NO_RESOURCES if malloc of the buffer fails.
  */
int MicroBitSerial::setTxBufferSize(uint8_t size)
{
    if(txInUse())
        return MICROBIT_SERIAL_IN_USE;

    lockTx();

    this->txBuffSize = size;

    int result = initialiseTx();

    unlockTx();

    return result;
}

/**
  * @return the current size of our rxBuff in bytes
  */
int MicroBitSerial::getRxBufferSize()
{
    return this->rxBuffSize;
}

/**
  * @return the current size of our txBuff in bytes
  */
int MicroBitSerial::getTxBufferSize()
{
    return this->txBuffSize;
}

/**
  * Sets the tail to match the head of our circular buffer for reception
  *
  * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
  *         for reception, otherwise MICROBIT_OK.
  */
int MicroBitSerial::clearRxBuffer()
{
    if(rxInUse())
        return MICROBIT_SERIAL_IN_USE;

    lockRx();

    rxBuffTail = rxBuffHead;

    unlockRx();

    return MICROBIT_OK;
}

/**
  * Sets the tail to match the head of our circular buffer for transmission
  *
  * @return MICROBIT_SERIAL_IN_USE if another fiber is currently using this instance
  *         for transmission, otherwise MICROBIT_OK.
  */
int MicroBitSerial::clearTxBuffer()
{
    if(txInUse())
        return MICROBIT_SERIAL_IN_USE;

    lockTx();

    txBuffTail = txBuffHead;

    unlockTx();

    return MICROBIT_OK;
}

/**
  * @return The currently buffered number of bytes in our rxBuff.
  */
int MicroBitSerial::rxBufferedSize()
{
    if(rxBuffTail > rxBuffHead)
        return (rxBuffSize - rxBuffTail) + rxBuffHead;

    return rxBuffHead - rxBuffTail;
}

/**
  * @return The currently buffered number of bytes in our txBuff.
  */
int MicroBitSerial::txBufferedSize()
{
    if(txBuffTail > txBuffHead)
        return (txBuffSize - txBuffTail) + txBuffHead;

    return txBuffHead - txBuffTail;
}

/**
  * @return The state of our mutex lock for reception.
  *
  * @note only one fiber can call read at a time
  */
int MicroBitSerial::rxInUse()
{
    return (status & MICROBIT_SERIAL_RX_IN_USE);
}

/**
  * @return The state of our mutex lock for transmition.
  *
  * @note only one fiber can call send at a time
  */
int MicroBitSerial::txInUse()
{
    return (status & MICROBIT_SERIAL_TX_IN_USE);
}

/**
  * Detaches a previously configured interrupt
  *
  * @param interruptType one of Serial::RxIrq or Serial::TxIrq
  *
  * @note #HACK, this should really be further up in the mbed libs, after
  *       attaching, you would expect to be able to detach...
  */
void MicroBitSerial::detach(Serial::IrqType interruptType)
{
    //we detach by sending a bad value to attach, for some weird reason...
    attach((MicroBitSerial *)NULL, &MicroBitSerial::dataReceived, interruptType);
}
