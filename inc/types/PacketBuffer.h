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

#ifndef MICROBIT_PACKET_BUFFER_H
#define MICROBIT_PACKET_BUFFER_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "RefCounted.h"

struct PacketData : RefCounted
{
    int             rssi;               // The radio signal strength this packet was received.
    uint8_t         length;             // The length of the payload in bytes
    uint8_t         payload[0];         // User / higher layer protocol data
};

/**
  * Class definition for a PacketBuffer.
  * A PacketBuffer holds a series of bytes that can be sent or received from the MicroBitRadio channel.
  *
  * @note This is a mutable, managed type.
  */
class PacketBuffer
{
    PacketData      *ptr;     // Pointer to payload data

    public:

    /**
      * Provide a pointer to a memory location containing the packet data.
      *
      * @return The contents of this packet, as an array of bytes.
      */
    uint8_t *getBytes();

    /**
      * Default Constructor.
      * Creates an empty Packet Buffer.
      *
      * @code
      * PacketBuffer p();
      * @endcode
      */
    PacketBuffer();

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
    PacketBuffer(int length);

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
    PacketBuffer(uint8_t *data, int length, int rssi = 0);

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
    PacketBuffer(const PacketBuffer &buffer);

    /**
      * Internal constructor-initialiser.
      *
      * @param data The data with which to fill the buffer.
      *
      * @param length The length of the buffer to create.
      *
      * @param rssi The radio signal strength at the time this packet was recieved.
      */
    void init(uint8_t *data, int length, int rssi);

    /**
      * Destructor.
      *
      * Removes buffer resources held by the instance.
      */
    ~PacketBuffer();

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
    PacketBuffer& operator = (const PacketBuffer& p);

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
    uint8_t operator [] (int i) const;

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
    uint8_t& operator [] (int i);

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
    bool operator== (const PacketBuffer& p);

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
    int setByte(int position, uint8_t value);

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
    int getByte(int position);

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
    int length();

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
    int getRSSI();

    /**
      * Sets the received signal strength of this packet.
      *
      * @code
      * PacketBuffer p1(16);
      * p1.setRSSI(37);
      * @endcode
      */
    void setRSSI(uint8_t rssi);

    static PacketBuffer EmptyPacket;
};

#endif
