#ifndef MICROBIT_RADIO_DATAGRAM_H
#define MICROBIT_RADIO_DATAGRAM_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitRadio.h"
#include "ManagedString.h"

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
class MicroBitRadioDatagram
{
    MicroBitRadio   &radio;     // The underlying radio module used to send and receive data.
    FrameBuffer     *rxQueue;   // A linear list of incoming packets, queued awaiting processing.

    public:

    /**
      * Constructor.
      *
      * Creates an instance of a MicroBitRadioDatagram which offers the ability
      * to broadcast simple text or binary messages to other micro:bits in the vicinity
      *
      * @param r The underlying radio module used to send and receive data.
      */
    MicroBitRadioDatagram(MicroBitRadio &r);

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
    int recv(uint8_t *buf, int len);

    /**
      * Retreives packet payload data into the given buffer.
      *
      * If a data packet is already available, then it will be returned immediately to the caller
      * in the form of a PacketBuffer.
      *
      * @return the data received, or an empty PacketBuffer if no data is available.
      */
    PacketBuffer recv();

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
    int send(uint8_t *buffer, int len);

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
    int send(PacketBuffer data);

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
    int send(ManagedString data);

    /**
      * Protocol handler callback. This is called when the radio receives a packet marked as a datagram.
      *
      * This function process this packet, and queues it for user reception.
      */
    void packetReceived();
};

#endif
