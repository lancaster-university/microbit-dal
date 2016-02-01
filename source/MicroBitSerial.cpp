#include "mbed.h"
#include "MicroBit.h"

uint8_t MicroBitSerial::status = 0;

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
  * @note the default baud rate is 115200. More API details can be found:
  *       -https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/api/SerialBase.h
  *       -https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/api/Serial.h
  */
MicroBitSerial::MicroBitSerial(PinName tx, PinName rx) : Serial(tx,rx)
{
    this->baud(MICROBIT_SERIAL_DEFAULT_BAUD_RATE);
}

/**
  * An internal interrupt callback for MicroBitSerial.
  *
  * Each time an interrupt occurs, a buffer is filled. When the
  * buffer is full, an event is fired which unblocks a waiting fiber,
  * which then handles the buffer.
  */
void MicroBitSerial::dataReceived()
{
    if(!(status & MICROBIT_SERIAL_STATE_IN_USE))
        return;

    char c = _getc();

    int delimeterOffset = 0;
    int delimLength = this->delimeters->length();

    //is this character a user selected delimeter?
    while(delimeterOffset < delimLength)
    {
        if(this->delimeters->charAt(delimeterOffset) == c)
        {
            MicroBitEvent(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_FIN_RCV);
            return;
        }

        delimeterOffset++;
    }

    *this->currentChar = c;

    this->currentChar++;

    //if we have exceeded the buffer size, unblock our fiber!
    if(this->currentChar > (this->buffer + this->stringLength))
        MicroBitEvent(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_FIN_RCV);
}

/**
  * An internal interrupt callback for MicroBitSerial.
  *
  * Each time an interrupt occurs, a buffer is filled. When the
  * buffer is full, an event is fired which unblocks a waiting fiber,
  * which then handles the buffer.
  */
void MicroBitSerial::dataWritten()
{
    if(!(status & MICROBIT_SERIAL_STATE_IN_USE))
        return;

    //if we have sent all our chars, unblock our fiber!
    if(this->currentChar > (this->buffer + this->stringLength))
        MicroBitEvent(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_FIN_TX);

    //send our current char
    _putc(*this->currentChar);

    this->currentChar++;
}

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
void MicroBitSerial::setWriteInterrupt(ManagedString string)
{
    //obtain mutex
    status |= MICROBIT_SERIAL_STATE_IN_USE;

    this->stringLength = string.length() - 1;

    this->buffer = (char *)string.toCharArray();

    this->currentChar = this->buffer;// + 1;

    //set the TX interrupt
    Serial::attach(this, &MicroBitSerial::dataWritten, TxIrq);

    //block!
    fiber_wait_for_event(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_FIN_TX);
}

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
void MicroBitSerial::setReadInterrupt(ManagedString delimeters, int len)
{
    //obtain mutex
    status |= MICROBIT_SERIAL_STATE_IN_USE;

    //configure for serial interrupt
    this->stringLength = len - 1;

    this->delimeters = new ManagedString(delimeters);

    this->buffer = (char *)malloc(len + 1);

    this->buffer[len] = '\0';

    this->currentChar = this->buffer;

    //set the receive interrupt
    Serial::attach(this, &MicroBitSerial::dataReceived, RxIrq);

    //block!
    fiber_wait_for_event(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_FIN_RCV);
}

/**
  * An internal method that resets the MicroBitSerial instance to an unused state.
  */
void MicroBitSerial::resetRead()
{
    //stop the serial interrupt
    detach(RxIrq);

    //reset our state
    this->stringLength = 0;
    delete this->delimeters;
    free(this->buffer);

    //release mutex
    status &= !MICROBIT_SERIAL_STATE_IN_USE;
}

/**
  * An internal method that resets the MicroBitSerial instance to an unused state.
  */
void MicroBitSerial::resetWrite()
{

    detach(TxIrq);

    //reset our state
    this->stringLength = 0;

    //release mutex
    status &= !MICROBIT_SERIAL_STATE_IN_USE;
}

/**
  * Sends a character over serial.
  *
  * @param c the character to send
  *
  * @note this call blocks the current fiber.
  *
  * Example:
  * @code
  * uBit.serial.send('a');
  * @endcode
  */
int MicroBitSerial::send(char c)
{
    if((status & MICROBIT_SERIAL_STATE_IN_USE))
        return MICROBIT_SERIAL_IN_USE;

    setWriteInterrupt(ManagedString(c));
    resetWrite();

    return MICROBIT_OK;
}

/**
  * Sends a managed string over serial.
  *
  * @param s the ManagedString to send
  *
  * @note this call blocks the current fiber.
  *
  * Example:
  * @code
  * uBit.serial.printString("abc123");
  * @endcode
  */
int MicroBitSerial::send(ManagedString s)
{
    if((status & MICROBIT_SERIAL_STATE_IN_USE))
        return MICROBIT_SERIAL_IN_USE;

    setWriteInterrupt(s);
    resetWrite();

    return MICROBIT_OK;
}

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
char MicroBitSerial::read()
{
    if((status & MICROBIT_SERIAL_STATE_IN_USE))
        return 0;

    setReadInterrupt("", 1);

    char c = this->buffer[0];

    resetRead();

    return c;
}

/**
  * Reads a sequence of characters from the serial bus.
  *
  * @param delimeters a series of delimeters that are evaluated on a per character
  *        basis.
  * @param len the length of the string you would like to read from the serial bus
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
ManagedString MicroBitSerial::read(int len, ManagedString delimeters)
{
    if(len < 2)
        return MICROBIT_INVALID_PARAMETER;

    if((status & MICROBIT_SERIAL_STATE_IN_USE))
        return MICROBIT_SERIAL_IN_USE;

    setReadInterrupt(delimeters, len);

    ManagedString toReturn(this->buffer);

    resetRead();

    return toReturn;
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
    Serial::attach((MicroBitSerial *)NULL, &MicroBitSerial::dataReceived, interruptType);
}
