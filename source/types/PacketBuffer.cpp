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

#include "MicroBitConfig.h"
#include "PacketBuffer.h"
#include "ErrorNo.h"

// Create the EmptyPacket reference.
PacketBuffer PacketBuffer::EmptyPacket = PacketBuffer(1);

/**
  * Default Constructor.
  * Creates an empty Packet Buffer.
  *
  * @code
  * PacketBuffer p();
  * @endcode
  */
PacketBuffer::PacketBuffer()
{
    this->init(NULL, 0, 0);
}

/**
  * Constructor.
  * Creates a new PacketBuffer of the given size.
  *
  * @param length The length of the buffer to create.
  *
  * @code
  * PacketBuffer p(16);         // Creates a PacketBuffer 16 bytes long.
  * @endcode
  */
PacketBuffer::PacketBuffer(int length)
{
    this->init(NULL, length, 0);
}

/**
  * Constructor.
  * Creates an empty Packet Buffer of the given size,
  * and fills it with the data provided.
  *
  * @param data The data with which to fill the buffer.
  *
  * @param length The length of the buffer to create.
  *
  * @param rssi The radio signal strength at the time this packet was recieved. Defaults to 0.
  *
  * @code
  * uint8_t buf = {13,5,2};
  * PacketBuffer p(buf, 3);         // Creates a PacketBuffer 3 bytes long.
  * @endcode
  */
PacketBuffer::PacketBuffer(uint8_t *data, int length, int rssi)
{
    this->init(data, length, rssi);
}

/**
  * Copy Constructor.
  * Add ourselves as a reference to an existing PacketBuffer.
  *
  * @param buffer The PacketBuffer to reference.
  *
  * @code
  * PacketBuffer p();
  * PacketBuffer p2(p); // Refers to the same packet as p.
  * @endcode
  */
PacketBuffer::PacketBuffer(const PacketBuffer &buffer)
{
    ptr = buffer.ptr;
    ptr->incr();
}

/**
  * Internal constructor-initialiser.
  *
  * @param data The data with which to fill the buffer.
  *
  * @param length The length of the buffer to create.
  *
  * @param rssi The radio signal strength at the time this packet was recieved.
  */
void PacketBuffer::init(uint8_t *data, int length, int rssi)
{
    if (length < 0)
        length = 0;

    ptr = (PacketData *) malloc(sizeof(PacketData) + length);
    ptr->init();

    ptr->length = length;
    ptr->rssi = rssi;

    // Copy in the data buffer, if provided.
    if (data)
        memcpy(ptr->payload, data, length);
}

/**
  * Destructor.
  *
  * Removes buffer resources held by the instance.
  */
PacketBuffer::~PacketBuffer()
{
    ptr->decr();
}

/**
  * Copy assign operation.
  *
  * Called when one PacketBuffer is assigned the value of another using the '=' operator.
  *
  * Decrements our reference count and free up the buffer as necessary.
  *
  * Then, update our buffer to refer to that of the supplied PacketBuffer,
  * and increase its reference count.
  *
  * @param p The PacketBuffer to reference.
  *
  * @code
  * uint8_t buf = {13,5,2};
  * PacketBuffer p1(16);
  * PacketBuffer p2(buf, 3);
  *
  * p1 = p2;
  * @endcode
  */
PacketBuffer& PacketBuffer::operator = (const PacketBuffer &p)
{
    if(ptr == p.ptr)
        return *this;

    ptr->decr();
    ptr = p.ptr;
    ptr->incr();

    return *this;
}

/**
  * Array access operation (read).
  *
  * Called when a PacketBuffer is dereferenced with a [] operation.
  *
  * Transparently map this through to the underlying payload for elegance of programming.
  *
  * @code
  * PacketBuffer p1(16);
  * uint8_t data = p1[0];
  * @endcode
  */
uint8_t PacketBuffer::operator [] (int i) const
{
    return ptr->payload[i];
}

/**
  * Array access operation (modify).
  *
  * Called when a PacketBuffer is dereferenced with a [] operation.
  *
  * Transparently map this through to the underlying payload for elegance of programming.
  *
  * @code
  * PacketBuffer p1(16);
  * p1[0] = 42;
  * @endcode
  */
uint8_t& PacketBuffer::operator [] (int i)
{
    return ptr->payload[i];
}

/**
  * Equality operation.
  *
  * Called when one PacketBuffer is tested to be equal to another using the '==' operator.
  *
  * @param p The PacketBuffer to test ourselves against.
  *
  * @return true if this PacketBuffer is identical to the one supplied, false otherwise.
  *
  * @code
  * MicroBitDisplay display;
  * uint8_t buf = {13,5,2};
  * PacketBuffer p1();
  * PacketBuffer p2();
  *
  * if(p1 == p2)                    // will be true
  *     display.scroll("same!");
  * @endcode
  */
bool PacketBuffer::operator== (const PacketBuffer& p)
{
    if (ptr == p.ptr)
        return true;
    else
        return (ptr->length == p.ptr->length && (memcmp(ptr->payload, p.ptr->payload, ptr->length)==0));
}

/**
  * Sets the byte at the given index to value provided.
  *
  * @param position The index of the byte to change.
  *
  * @param value The new value of the byte (0-255).
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * PacketBuffer p1(16);
  * p1.setByte(0,255);              // Sets the first byte in the buffer to the value 255.
  * @endcode
  */
int PacketBuffer::setByte(int position, uint8_t value)
{
    if (position < ptr->length)
    {
        ptr->payload[position] = value;
        return MICROBIT_OK;
    }
    else
    {
        return MICROBIT_INVALID_PARAMETER;
    }
}

/**
  * Determines the value of the given byte in the packet.
  *
  * @param position The index of the byte to read.
  *
  * @return The value of the byte at the given position, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * PacketBuffer p1(16);
  * p1.setByte(0,255);              // Sets the first byte in the buffer to the value 255.
  * p1.getByte(0);                  // Returns 255.
  * @endcode
  */
int PacketBuffer::getByte(int position)
{
    if (position < ptr->length)
        return ptr->payload[position];
    else
        return MICROBIT_INVALID_PARAMETER;
}

/**
  * Provide a pointer to a memory location containing the packet data.
  *
  * @return The contents of this packet, as an array of bytes.
  */
uint8_t*PacketBuffer::getBytes()
{
    return ptr->payload;
}

/**
  * Gets number of bytes in this buffer
  *
  * @return The size of the buffer in bytes.
  *
  * @code
  * PacketBuffer p1(16);
  * p1.length(); // Returns 16.
  * @endcode
  */
int PacketBuffer::length()
{
    return ptr->length;
}

/**
  * Retrieves the received signal strength of this packet.
  *
  * @return The signal strength of the radio when this packet was received, in -dbm.
  * The higher the value, the stronger the signal. Typical values are in the range -42 to -128.
  *
  * @code
  * PacketBuffer p1(16);
  * p1.getRSSI();                 // Returns the received signal strength.
  * @endcode
  */
int PacketBuffer::getRSSI()
{
    return ptr->rssi;
}

/**
  * Sets the received signal strength of this packet.
  *
  * @code
  * PacketBuffer p1(16);
  * p1.setRSSI(37);
  * @endcode
  */
void PacketBuffer::setRSSI(uint8_t rssi)
{
    ptr->rssi = rssi;
}
