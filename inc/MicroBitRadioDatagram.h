#ifndef MICROBIT_RADIO_DATAGRAM_H
#define MICROBIT_RADIO_DATAGRAM_H

#include "mbed.h"
#include "MicroBitRadio.h"

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

class MicroBitRadioDatagram 
{
    FrameBuffer    *rxQueue;   // A linear list of incoming packets, queued awaiting processing.

    public:

    /**
     * Constructor.
     */
    MicroBitRadioDatagram();

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
    int recv(uint8_t *buf, int len);

    /**
     * Retreives packet payload data into the given buffer.
     * If a data packet is already available, then it will be returned immediately to the caller,
     * in the form of a string. If no data is available the EmptyString is returned.
     *
     * @return the data received, or the EmptyString if no data is available.
     */
    PacketBuffer recv();

    /**
     * Transmits the given buffer onto the broadcast radio.
     * The call will wait until the transmission of the packet has completed before returning.
     *
     * @param buffer The packet contents to transmit.
     * @param len The number of bytes to transmit.
     * @return MICROBIT_OK on success.
     */
    int send(uint8_t *buffer, int len);

    /**
     * Transmits the given string onto the broadcast radio.
     * The call will wait until the transmission of the packet has completed before returning.
     *
     * @param data The packet contents to transmit.
     * @return MICROBIT_OK on success.
     */
    int send(PacketBuffer data);

    /**
     * Protocol handler callback. This is called when the radio receives a packet marked as a datagram.
     * This function process this packet, and queues it for user reception.
     */
    void packetReceived();
};

#endif
