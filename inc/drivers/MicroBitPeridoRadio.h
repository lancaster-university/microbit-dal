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

#ifndef MICROBIT_PERIDO_RADIO_H
#define MICROBIT_PERIDO_RADIO_H

class MicroBitPeridoRadio;
struct PeridoFrameBuffer;

#include "mbed.h"
#include "MicroBitConfig.h"
#include "PacketBuffer.h"
#include "MicroBitRadio.h"
#include "LowLevelTimer.h"
#include "HigherLevelTimer.h"
#include "ManagedString.h"
#include "PeridoRadioCloud.h"

/**
 * Provides a simple broadcast radio abstraction, built upon the raw nrf51822 RADIO module.
 *
 * The nrf51822 RADIO module supports a number of proprietary modes of operation in addition to the typical BLE usage.
 * This class uses one of these modes to enable simple, point to multipoint communication directly between micro:bits.
 *
 * TODO: The protocols implemented here do not currently perform any significant form of energy management,
 * which means that they will consume far more energy than their BLE equivalent. Later versions of the protocol
 * should look to address this through energy efficient broadcast techniques / sleep scheduling. In particular, the GLOSSY
 * approach to efficienct rebroadcast and network synchronisation would likely provide an effective future step.
 *
 * TODO: Meshing should also be considered - again a GLOSSY approach may be effective here, and highly complementary to
 * the master/slave arachitecture of BLE.
 *
 * TODO: This implementation only operates whilst the BLE stack is disabled. The nrf51822 provides a timeslot API to allow
 * BLE to cohabit with other protocols. Future work to allow this colocation would be benefical, and would also allow for the
 * creation of wireless BLE bridges.
 *
 * NOTE: This API does not contain any form of encryption, authentication or authorization. It's purpose is solely for use as a
 * teaching aid to demonstrate how simple communications operates, and to provide a sandpit through which learning can take place.
 * For serious applications, BLE should be considered a substantially more secure alternative.
 */


#define MICROBIT_RADIO_MAXIMUM_RX_BUFFERS       4
#define MICROBIT_RADIO_STATUS_INITIALISED       0x0001
#define MICROBIT_RADIO_DEFAULT_TX_POWER         6
#define MICROBIT_RADIO_DEFAULT_FREQUENCY        7
#define MICROBIT_RADIO_BASE_ADDRESS             0x75626975

// Default configuration values
#define MICROBIT_PERIDO_HEADER_SIZE             10
#define MICROBIT_PERIDO_DEFAULT_SLEEP           600

#define MICROBIT_PERIDO_MAX_PACKET_SIZE         200

#define MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS      10

#define MICROBIT_PERIDO_DEFAULT_APP_ID          0
#define MICROBIT_PERIDO_DEFAULT_NAMESPACE       0

#define MICROBIT_PERIDO_FRAME_PROPOSAL_FLAG     0x01
#define MICROBIT_PERIDO_FRAME_KEEP_ALIVE_FLAG     0x02

struct PeridoFrameBuffer
{
    uint8_t             length;                             // The length of the remaining bytes in the packet.
    uint8_t             app_id;
    uint8_t             namespace_id;
    uint16_t            id;
    uint8_t             ttl:4, initial_ttl:4;
    uint32_t            time_since_wake:24, period:4, flags:4;
    uint8_t             payload[MICROBIT_PERIDO_MAX_PACKET_SIZE];    // User / higher layer protocol data
} __attribute__((packed));


class MicroBitPeridoRadio : public MicroBitComponent
{
    uint8_t                appId;
    uint8_t                namespaceId;

    public:

    uint8_t                 periodIndex;
    uint8_t                 rxQueueDepth; // The number of packets in the receiver queue.
    uint8_t                 txQueueDepth; // The number of packets in the tx queue.

    LowLevelTimer&          timer;
    PeridoRadioCloud        cloud;       // A simple REST handling service.

    // a fifo array of received packets
    // the array can hold a maximum of MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS - 1 packets
    PeridoFrameBuffer       *rxArray[MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS];
    uint8_t                 rxHead; // head points to the first rx'd packet-1
    uint8_t                 rxTail; // tail points to the last rx'd packet

    // a fifo array of transmitted packets
    // the array can hold a maximum of MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS - 1 packets
    PeridoFrameBuffer       *txArray[MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS];
    uint8_t                 txHead; // head points to the first packet to be tx'd
    uint8_t                 txTail; // head points to the last packet to be tx'd

    // this member variable is allocated and used whenever a packet is received. The received packet is then copied into the rxArray
    PeridoFrameBuffer       *rxBuf;

    static MicroBitPeridoRadio    *instance;  // A singleton reference, used purely by the interrupt service routine.

    /**
      * Constructor.
      *
      * Initialise the MicroBitPeridoRadio.
      *
      * @note This class is demand activated, as a result most resources are only
      *       committed if send/recv or event registrations calls are made.
      */
    MicroBitPeridoRadio(LowLevelTimer& timer, uint8_t appId = MICROBIT_PERIDO_DEFAULT_APP_ID, uint8_t namespaceId = MICROBIT_PERIDO_DEFAULT_NAMESPACE, uint16_t id = MICROBIT_ID_RADIO);

    /**
      * Change the output power level of the transmitter to the given value.
      *
      * @param power a value in the range 0..7, where 0 is the lowest power and 7 is the highest.
      *
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the value is out of range.
      */
    int setTransmitPower(int power);

    /**
      * Change the transmission and reception band of the radio to the given channel
      *
      * @param band a frequency band in the range 0 - 100. Each step is 1MHz wide, based at 2400MHz.
      *
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the value is out of range,
      *         or MICROBIT_NOT_SUPPORTED if the BLE stack is running.
      */
    int setFrequencyBand(int band);

    /**
      * Retrieve a pointer to the currently allocated receive buffer. This is the area of memory
      * actively being used by the radio hardware to store incoming data.
      *
      * @return a pointer to the current receive buffer.
      */
    PeridoFrameBuffer * getRxBuf();

    int popTxQueue();

    PeridoFrameBuffer* getTxBuf();

    /**
      * Attempt to queue a buffer received by the radio hardware, if sufficient space is available.
      *
      * @return MICROBIT_OK on success, or MICROBIT_NO_RESOURCES if a replacement receiver buffer
      *         could not be allocated (either by policy or memory exhaustion).
      */
    int copyRxBuf();

    int queueTxBuf(PeridoFrameBuffer* tx);

    PeridoFrameBuffer* getCurrentTxBuf();

    int queueKeepAlive();

    /**
      * Initialises the radio for use as a multipoint sender/receiver
      *
      * @return MICROBIT_OK on success, MICROBIT_NOT_SUPPORTED if the BLE stack is running.
      */
    int enable();

    /**
      * Disables the radio for use as a multipoint sender/receiver.
      *
      * @return MICROBIT_OK on success, MICROBIT_NOT_SUPPORTED if the BLE stack is running.
      */
    int disable();

    /**
      * Sets the radio to listen to packets sent with the given group id.
      *
      * @param group The group to join. A micro:bit can only listen to one group ID at any time.
      *
      * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the BLE stack is running.
      */
    int setGroup(uint8_t group);

    /**
      * Set the current period in milliseconds broadcasted in the perido frame
      *
      * @param period_ms the new period, in milliseconds.
      *
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the period is too short.
      */
    int setPeriod(uint32_t period_ms);

    /**
      * Retrieve the current period in milliseconds broadcasted in the perido frame
      *
      * @return the current period in milliseconds
      */
    uint32_t getPeriod();

    /**
      * Determines the number of packets ready to be processed.
      *
      * @return The number of packets in the receive buffer.
      */
    int dataReady();

    /**
      * Retrieves the next packet from the receive buffer.
      * If a data packet is available, then it will be returned immediately to
      * the caller. This call will also dequeue the buffer.
      *
      * @return The buffer containing the the packet. If no data is available, NULL is returned.
      *
      * @note Once recv() has been called, it is the callers responsibility to
      *       delete the buffer when appropriate.
      */
    PeridoFrameBuffer* recv();

    PeridoFrameBuffer* peakRxQueue();

    void idleTick();

    int setAppId(uint16_t id);

    int getAppId();

    /**
      * Transmits the given buffer onto the broadcast radio.
      * The call will wait until the transmission of the packet has completed before returning.
      *
      * @param data The packet contents to transmit.
      *
      * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the BLE stack is running.
      */
    int send(PeridoFrameBuffer* buffer);

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
     * Generates an id based on historic information.
     **/
    uint16_t generateId(uint8_t app_id, uint8_t namespace_id);

};

#endif
