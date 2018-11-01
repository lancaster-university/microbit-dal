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
#if MICROBIT_RADIO_VERSION == MICROBIT_RADIO_PERIDO

#include "MicroBitPeridoRadio.h"
#include "ErrorNo.h"
#include "MicroBitCompat.h"

/**
  * Provides a simple broadcast radio abstraction, built upon the raw nrf51822 RADIO module.
  *
  * This class provides the ability to broadcast simple text or binary messages to other micro:bits in the vicinity
  * It is envisaged that this would provide the basis for children to experiment with building their own, simple,
  * custom protocols.
  *
  * @note This API does not contain any form of encryption, authentication or authorisation. Its purpose is solely for use as a
  * teaching aid to demonstrate how simple communications operates, and to provide a sandpit through which learning can take place.
  * For serious applications, BLE should be considered a substantially more secure alternative.
  */

/**
* Constructor.
*
* Creates an instance of a PeridoRadioDatagram which offers the ability
* to broadcast simple text or binary messages to other micro:bits in the vicinity
*
* @param r The underlying radio module used to send and receive data.
*/
PeridoRadioDatagram::PeridoRadioDatagram(MicroBitPeridoRadio &r, uint8_t namespaceId) : radio(r)
{
    memset(this->rxArray, 0, sizeof(PeridoFrameBuffer*) * PERIDO_RADIO_DATAGRAM_MAX_PACKETS);
    this->namespaceId = namespaceId;
    this->rxTail = 0;
    this->rxHead = 0;
}

/**
  * Retrieves packet payload data into the given buffer.
  *
  * If a data packet is already available, then it will be returned immediately to the caller.
  * If no data is available then MICROBIT_INVALID_PARAMETER is returned.
  *
  * @param buf A pointer to a valid memory location where the received data is to be stored
  *
  * @param len The maximum amount of data that can safely be stored in 'buf'
  *
  * @return The length of the data stored, or MICROBIT_INVALID_PARAMETER if no data is available, or the memory regions provided are invalid.
  */
int PeridoRadioDatagram::recv(uint8_t *buf, int len)
{
    if (buf == NULL || this->rxTail == this->rxHead || len < 0)
        return MICROBIT_INVALID_PARAMETER;

    uint8_t nextHead = (this->rxHead + 1) % PERIDO_RADIO_DATAGRAM_MAX_PACKETS;

    PeridoFrameBuffer *p = rxArray[nextHead];
    this->rxArray[nextHead] = NULL;
    this->rxHead = nextHead;

    int l = min(len, p->length - (MICROBIT_PERIDO_HEADER_SIZE - 1));

    // Fill in the buffer provided, if possible.
    memcpy(buf, p->payload, l);

    delete p;
    return l;
}

/**
  * Retreives packet payload data into the given buffer.
  *
  * If a data packet is already available, then it will be returned immediately to the caller
  * in the form of a PacketBuffer.
  *
  * @return the data received, or an empty PacketBuffer if no data is available.
  */
PacketBuffer PeridoRadioDatagram::recv()
{
    if (this->rxTail == this->rxHead)
        return PacketBuffer::EmptyPacket;

    uint8_t nextHead = (this->rxHead + 1) % PERIDO_RADIO_DATAGRAM_MAX_PACKETS;

    PeridoFrameBuffer *p = rxArray[nextHead];
    this->rxArray[nextHead] = NULL;
    this->rxHead = nextHead;

    PacketBuffer packet(p->payload, p->length - (MICROBIT_PERIDO_HEADER_SIZE - 1), 0);
    delete p;

    return packet;
}

/**
  * Transmits the given buffer onto the broadcast radio.
  *
  * This is a synchronous call that will wait until the transmission of the packet
  * has completed before returning.
  *
  * @param buffer The packet contents to transmit.
  *
  * @param len The number of bytes to transmit.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the buffer is invalid,
  *         or the number of bytes to transmit is greater than `MICROBIT_RADIO_MAX_PACKET_SIZE + MICROBIT_RADIO_HEADER_SIZE`.
  */
int PeridoRadioDatagram::send(uint8_t *buffer, int len)
{
    if (buffer == NULL || len < 0 || len > MICROBIT_PERIDO_MAX_PACKET_SIZE + MICROBIT_PERIDO_HEADER_SIZE - 1)
        return MICROBIT_INVALID_PARAMETER;

    return radio.send(buffer, len, this->namespaceId);
}

/**
  * Transmits the given string onto the broadcast radio.
  *
  * This is a synchronous call that will wait until the transmission of the packet
  * has completed before returning.
  *
  * @param data The packet contents to transmit.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the buffer is invalid,
  *         or the number of bytes to transmit is greater than `MICROBIT_RADIO_MAX_PACKET_SIZE + MICROBIT_RADIO_HEADER_SIZE`.
  */
int PeridoRadioDatagram::send(PacketBuffer data)
{
    return send((uint8_t *)data.getBytes(), data.length());
}

/**
  * Transmits the given string onto the broadcast radio.
  *
  * This is a synchronous call that will wait until the transmission of the packet
  * has completed before returning.
  *
  * @param data The packet contents to transmit.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the buffer is invalid,
  *         or the number of bytes to transmit is greater than `MICROBIT_RADIO_MAX_PACKET_SIZE + MICROBIT_RADIO_HEADER_SIZE`.
  */
int PeridoRadioDatagram::send(ManagedString data)
{
    return send((uint8_t *)data.toCharArray(), data.length());
}

/**
  * Protocol handler callback. This is called when the radio receives a packet marked as a datagram.
  *
  * This function process this packet, and queues it for user reception.
  */
extern void log_string_ch(const char *);
void PeridoRadioDatagram::packetReceived()
{
    uint8_t nextTail = (this->rxTail + 1) % PERIDO_RADIO_DATAGRAM_MAX_PACKETS;
    PeridoFrameBuffer *packet = radio.recv();

    log_string_ch("DATAGRAM REC");

    // no room
    if (nextTail == this->rxHead)
    {
        delete packet;
        return;
    }

    // add our buffer to the array before updating the head
    // this ensures atomicity.
    this->rxArray[nextTail] = packet;
    this->rxTail = nextTail;

    MicroBitEvent(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM);
}

uint8_t PeridoRadioDatagram::getNamespaceId()
{
    return this->namespaceId;
}

#endif