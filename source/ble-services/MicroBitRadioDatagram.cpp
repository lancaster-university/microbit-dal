#include "MicroBit.h"

/**
  * Provides a simple broadcast radio abstraction, built upon the raw nrf51822 RADIO module.
  *
  * This class provides the ability to broadcast simple text or binary messages to other micro:bits in the vicinity
  * It is envisaged that this would provide the basis for children to experiment with building their own, simple,
  * custom protocols.
  *
  * NOTE: This API does not contain any form of encryption, authentication or authorisation. Its purpose is solely for use as a
  * teaching aid to demonstrate how simple communications operates, and to provide a sandpit through which learning can take place.
  * For serious applications, BLE should be considered a substantially more secure alternative.
  */

/**
  * Constructor.
  */
MicroBitRadioDatagram::MicroBitRadioDatagram()
{
    rxQueue = NULL;
}

/**
 * Retreives packet payload data into the given buffer.
 * If a data packet is already available, then it will be returned immediately to the caller. 
 * If no data is available the EmptyString is returned, then MICROBIT_INVALID_PARAMETER is returned.
 *
 * @param buf A pointer to a valid memory location where the received data is to be stored.
 * @param len The maximum amount of data that can safely be stored in 'buf'
 *
 * @return The length of the data stored, or MICROBIT_INVALID_PARAMETER if no data is available, or the memory regions provided are invalid.
 */
int MicroBitRadioDatagram::recv(uint8_t *buf, int len)
{
    if (buf == NULL || rxQueue == NULL || len < 0)
        return MICROBIT_INVALID_PARAMETER;

    // Take the first buffer from the queue.
    FrameBuffer *p = rxQueue;
    rxQueue = rxQueue->next;

    int l = min(len, p->length - (MICROBIT_RADIO_HEADER_SIZE - 1));

    // Fill in the buffer provided, if possible.
    memcpy(buf, p->payload, l);

    delete p;
    return l;
}

/**
 * Retreives packet payload data into the given buffer.
 * If a data packet is already available, then it will be returned immediately to the caller,
 * in the form of a string. If no data is available the EmptyString is returned.
 *
 * @return the data received, or the EmptyString if no data is available.
 */
PacketBuffer MicroBitRadioDatagram::recv()
{
    if (rxQueue == NULL)
        return PacketBuffer::EmptyPacket;

    FrameBuffer *p = rxQueue;
    rxQueue = rxQueue->next;

    PacketBuffer packet(p->payload, p->length - (MICROBIT_RADIO_HEADER_SIZE - 1), p->rssi);

    delete p;
    return packet;
}

/**
 * Transmits the given buffer onto the broadcast radio.
 * The call will wait until the transmission of the packet has completed before returning.
 *
 * @param buffer The packet contents to transmit.
 * @param len The number of bytes to transmit.
 * @return MICROBIT_OK on success.
 */
int MicroBitRadioDatagram::send(uint8_t *buffer, int len)
{
    if (buffer == NULL || len < 0 || len > MICROBIT_RADIO_MAX_PACKET_SIZE + MICROBIT_RADIO_HEADER_SIZE - 1)
        return MICROBIT_INVALID_PARAMETER;
 
    FrameBuffer buf;

    buf.length = len + MICROBIT_RADIO_HEADER_SIZE - 1;
    buf.version = 1;
    buf.group = 0;
    buf.protocol = MICROBIT_RADIO_PROTOCOL_DATAGRAM;
    memcpy(buf.payload, buffer, len);

    return uBit.radio.send(&buf);
}

/**
 * Transmits the given string onto the broadcast radio.
 * The call will wait until the transmission of the packet has completed before returning.
 *
 * @param data The packet contents to transmit.
 * @return MICROBIT_OK on success.
 */
int MicroBitRadioDatagram::send(PacketBuffer data)
{
    return send((uint8_t *)data.getBytes(), data.length());
}

/**
 * Protocol handler callback. This is called when the radio receives a packet marked as a datagram.
 * This function process this packet, and queues it for user reception.
 */
void MicroBitRadioDatagram::packetReceived()
{
    FrameBuffer *packet = uBit.radio.recv();
    int queueDepth = 0;

    // We add to the tail of the queue to preserve causal ordering.
    packet->next = NULL;

    if (rxQueue == NULL)
    {
        rxQueue = packet;
    }
    else
    {
        FrameBuffer *p = rxQueue;
        while (p->next != NULL)
        {
            p = p->next;
            queueDepth++;
        }

        if (queueDepth >= MICROBIT_RADIO_MAXIMUM_RX_BUFFERS)
        {
            delete packet;
            return;
        } 

        p->next = packet;
    }

    MicroBitEvent(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM);
}

