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
#include "MicroBitPeridoRadio.h"
#include "MicroBitComponent.h"
#include "EventModel.h"
#include "MicroBitDevice.h"
#include "ErrorNo.h"
#include "MicroBitFiber.h"
#include "MicroBitBLEManager.h"

/**
  * Provides a simple broadcast radio abstraction, built upon the raw nrf51822 RADIO module.
  *
  * The nrf51822 RADIO module supports a number of proprietary modes of operation in addition to the typical BLE usage.
  * This class uses one of these modes to enable simple, point to multipoint communication directly between micro:bits.
  *
  * TODO: The protocols implemented here do not currently perform any significant form of energy management,
  * which means that they will consume far more energy than their BLE equivalent. Later versions of the protocol
  * should look to address this through energy efficient broadcast techniques / sleep scheduling. In particular, the GLOSSY
  * approach to efficient rebroadcast and network synchronisation would likely provide an effective future step.
  *
  * TODO: Meshing should also be considered - again a GLOSSY approach may be effective here, and highly complementary to
  * the master/slave arachitecture of BLE.
  *
  * TODO: This implementation may only operated whilst the BLE stack is disabled. The nrf51822 provides a timeslot API to allow
  * BLE to cohabit with other protocols. Future work to allow this colocation would be benefical, and would also allow for the
  * creation of wireless BLE bridges.
  *
  * NOTE: This API does not contain any form of encryption, authentication or authorisation. Its purpose is solely for use as a
  * teaching aid to demonstrate how simple communications operates, and to provide a sandpit through which learning can take place.
  * For serious applications, BLE should be considered a substantially more secure alternative.
  */

MicroBitPeridoRadio* MicroBitPeridoRadio::instance = NULL;

#define MICROBIT_TRANSMITTER        0
#define MICROBIT_RECEIVER        0

extern void set_gpio(int);

extern void valid_packet_received(PeridoFrameBuffer*);
extern void increment_counter(int);

#define RADIO_STATUS_RECV_EN        0x00000001
#define RADIO_STATUS_RECV_RDY       0x00000002
#define RADIO_STATUS_RECV_END       0x00000004

#define RADIO_STATUS_TX_EN          0x00000008
#define RADIO_STATUS_TX_RDY         0x00000010

#define RADIO_STATUS_TX_ST          0x00000020
#define RADIO_STATUS_TX_END         0x00000040

#define RADIO_STATUS_DISABLED       0x00000080
#define RADIO_STATUS_RETRANS        0x00000100

#define LAST_SEEN_BUFFER_SIZE       3

uint32_t radio_status = 0;

uint8_t last_seen_index = 0;
uint32_t last_seen[LAST_SEEN_BUFFER_SIZE] = { 0 };

void retransmit()
{
    set_gpio(1);

    // // // Turn off the transceiver.
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    for (volatile int i =0; i < 131; i++);

    radio_status = RADIO_STATUS_DISABLED;

    // Configure the radio to send the buffer provided.
    NRF_RADIO->PACKETPTR = (uint32_t) MicroBitPeridoRadio::instance->rxBuf;

    // Turn on the transmitter, and wait for it to signal that it's ready to use.
    radio_status = RADIO_STATUS_TX_EN;
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_TXEN = 1;
    for (volatile int i =0; i < 522; i++);
    radio_status = RADIO_STATUS_TX_RDY;

    set_gpio(0);

    set_gpio(1);
    radio_status = RADIO_STATUS_TX_ST;
    // Start transmission and wait for end of packet.
    NRF_RADIO->TASKS_START = 1;
    NRF_RADIO->EVENTS_END = 0;
    while(NRF_RADIO->EVENTS_END == 0);
    radio_status = RADIO_STATUS_TX_END;
    set_gpio(0);

    set_gpio(1);
    // Turn off the transmitter.
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while(NRF_RADIO->EVENTS_DISABLED == 0);
    radio_status = RADIO_STATUS_DISABLED;

    // Start listening for the next packet
    radio_status = RADIO_STATUS_RECV_EN;
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_RXEN = 1;
    while(NRF_RADIO->EVENTS_READY == 0);

    radio_status = RADIO_STATUS_RECV_RDY;

    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_END = 0;
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);

    // Re-enable the Radio interrupt.
    set_gpio(0);
}

extern "C" void RADIO_IRQHandler(void)
{
    if(NRF_RADIO->EVENTS_READY)
    {
        NRF_RADIO->EVENTS_READY = 0;

        if(radio_status & RADIO_STATUS_RECV_EN)
            radio_status = RADIO_STATUS_RECV_RDY;

        if(radio_status & RADIO_STATUS_TX_EN)
            radio_status = RADIO_STATUS_TX_RDY;

        // Start listening and wait for the END event
        NRF_RADIO->TASKS_START = 1;
    }

    if(NRF_RADIO->EVENTS_END)
    {
        NRF_RADIO->EVENTS_END = 0;
        if(radio_status & RADIO_STATUS_RECV_RDY)
        {
            if(NRF_RADIO->CRCSTATUS == 1)
            {
                PeridoFrameBuffer *p = MicroBitPeridoRadio::instance->rxBuf;

                if (p)
                {
                    if(p->ttl > 0)
                    {
                        p->ttl--;
                        retransmit();
                    }

                    bool seen = false;

                    // check if we've seen this ID before...
                    for (int i = 0; i < LAST_SEEN_BUFFER_SIZE; i++)
                        if(last_seen[i] == p->id)
                        {
                            seen = true;
                            increment_counter(i);
                        }

                    // if seen, queue a new buffer, and mark it as seen
                    if(!seen)
                    {
                        MicroBitPeridoRadio::instance->queueRxBuf();
                        NRF_RADIO->PACKETPTR = (uint32_t) MicroBitPeridoRadio::instance->getRxBuf();

                        valid_packet_received(MicroBitPeridoRadio::instance->recv());

                        last_seen[last_seen_index] = p->id;
                        last_seen_index = (last_seen_index + 1) %  LAST_SEEN_BUFFER_SIZE;
                    }
                }
            }

            // Start listening and wait for the END event
            NRF_RADIO->TASKS_START = 1;
        }
    }
}

/**
  * Constructor.
  *
  * Initialise the MicroBitPeridoRadio.
  *
  * @note This class is demand activated, as a result most resources are only
  *       committed if send/recv or event registrations calls are made.
  */
MicroBitPeridoRadio::MicroBitPeridoRadio(uint16_t id)
{
    this->id = id;
    this->status = 0;
	this->group = MICROBIT_RADIO_DEFAULT_GROUP;
	this->queueDepth = 0;
    this->rssi = 0;
    this->rxQueue = NULL;
    this->rxBuf = NULL;

    this->sleepPeriodMs = MICROBIT_PERIDO_DEFAULT_SLEEP;

    instance = this;
}

/**
  * Change the output power level of the transmitter to the given value.
  *
  * @param power a value in the range 0..7, where 0 is the lowest power and 7 is the highest.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the value is out of range.
  */
int MicroBitPeridoRadio::setTransmitPower(int power)
{
    if (power < 0 || power >= MICROBIT_BLE_POWER_LEVELS)
        return MICROBIT_INVALID_PARAMETER;

    NRF_RADIO->TXPOWER = (uint32_t)MICROBIT_BLE_POWER_LEVEL[power];

    return MICROBIT_OK;
}

/**
  * Change the transmission and reception band of the radio to the given channel
  *
  * @param band a frequency band in the range 0 - 100. Each step is 1MHz wide, based at 2400MHz.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the value is out of range,
  *         or MICROBIT_NOT_SUPPORTED if the BLE stack is running.
  */
int MicroBitPeridoRadio::setFrequencyBand(int band)
{
    if (ble_running())
        return MICROBIT_NOT_SUPPORTED;

    if (band < 0 || band > 100)
        return MICROBIT_INVALID_PARAMETER;

    NRF_RADIO->FREQUENCY = (uint32_t)band;

    return MICROBIT_OK;
}

/**
  * Retrieve a pointer to the currently allocated receive buffer. This is the area of memory
  * actively being used by the radio hardware to store incoming data.
  *
  * @return a pointer to the current receive buffer.
  */
PeridoFrameBuffer* MicroBitPeridoRadio::getRxBuf()
{
    return rxBuf;
}

/**
  * Attempt to queue a buffer received by the radio hardware, if sufficient space is available.
  *
  * @return MICROBIT_OK on success, or MICROBIT_NO_RESOURCES if a replacement receiver buffer
  *         could not be allocated (either by policy or memory exhaustion).
  */
int MicroBitPeridoRadio::queueRxBuf()
{
    if (rxBuf == NULL)
        return MICROBIT_INVALID_PARAMETER;

    if (queueDepth >= MICROBIT_RADIO_MAXIMUM_RX_BUFFERS)
        return MICROBIT_NO_RESOURCES;

    // Store the received RSSI value in the frame
    rxBuf->rssi = getRSSI();

    // Ensure that a replacement buffer is available before queuing.
    PeridoFrameBuffer *newRxBuf = new PeridoFrameBuffer();

    if (newRxBuf == NULL)
        return MICROBIT_NO_RESOURCES;

    // We add to the tail of the queue to preserve causal ordering.
    rxBuf->next = NULL;

    if (rxQueue == NULL)
    {
        rxQueue = rxBuf;
    }
    else
    {
        PeridoFrameBuffer *p = rxQueue;
        while (p->next != NULL)
            p = p->next;

        p->next = rxBuf;
    }

    // Increase our received packet count
    queueDepth++;

    // Allocate a new buffer for the receiver hardware to use. the old on will be passed on to higher layer protocols/apps.
    rxBuf = newRxBuf;

    return MICROBIT_OK;
}

/**
  * Sets the RSSI for the most recent packet.
  * The value is measured in -dbm. The higher the value, the stronger the signal.
  * Typical values are in the range -42 to -128.
  *
  * @param rssi the new rssi value.
  *
  * @note should only be called from RADIO_IRQHandler...
  */
int MicroBitPeridoRadio::setRSSI(int rssi)
{
    if (!(status & MICROBIT_RADIO_STATUS_INITIALISED))
        return MICROBIT_NOT_SUPPORTED;

    this->rssi = rssi;

    return MICROBIT_OK;
}

/**
  * Retrieves the current RSSI for the most recent packet.
  * The return value is measured in -dbm. The higher the value, the stronger the signal.
  * Typical values are in the range -42 to -128.
  *
  * @return the most recent RSSI value or MICROBIT_NOT_SUPPORTED if the BLE stack is running.
  */
int MicroBitPeridoRadio::getRSSI()
{
    if (!(status & MICROBIT_RADIO_STATUS_INITIALISED))
        return MICROBIT_NOT_SUPPORTED;

    return this->rssi;
}

/**
  * Initialises the radio for use as a multipoint sender/receiver
  *
  * @return MICROBIT_OK on success, MICROBIT_NOT_SUPPORTED if the BLE stack is running.
  */
int MicroBitPeridoRadio::enable()
{
    // If the device is already initialised, then there's nothing to do.
    if (status & MICROBIT_RADIO_STATUS_INITIALISED)
        return MICROBIT_OK;

    // Only attempt to enable this radio mode if BLE is disabled.
    if (ble_running())
        return MICROBIT_NOT_SUPPORTED;

    // If this is the first time we've been enable, allocate out receive buffers.
    if (rxBuf == NULL)
        rxBuf = new PeridoFrameBuffer();

    if (rxBuf == NULL)
        return MICROBIT_NO_RESOURCES;

    // Enable the High Frequency clock on the processor. This is a pre-requisite for
    // the RADIO module. Without this clock, no communication is possible.
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    // Bring up the nrf51822 RADIO module in Nordic's proprietary 1MBps packet radio mode.
    setTransmitPower(MICROBIT_RADIO_DEFAULT_TX_POWER);
    setFrequencyBand(MICROBIT_RADIO_DEFAULT_FREQUENCY);

    // Configure for 1Mbps throughput.
    // This may sound excessive, but running a high data rates reduces the chances of collisions...
    NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_1Mbit;

    // Configure the addresses we use for this protocol. We run ANONYMOUSLY at the core.
    // A 40 bit addresses is used. The first 32 bits match the ASCII character code for "uBit".
    // Statistically, this provides assurance to avoid other similar 2.4GHz protocols that may be in the vicinity.
    // We also map the assigned 8-bit GROUP id into the PREFIX field. This allows the RADIO hardware to perform
    // address matching for us, and only generate an interrupt when a packet matching our group is received.
    NRF_RADIO->BASE0 = MICROBIT_RADIO_BASE_ADDRESS;

    // Join the default group. This will configure the remaining byte in the RADIO hardware module.
    setGroup(this->group);

    // The RADIO hardware module supports the use of multiple addresses, but as we're running anonymously, we only need one.
    // Configure the RADIO module to use the default address (address 0) for both send and receive operations.
    NRF_RADIO->TXADDRESS = 0;
    NRF_RADIO->RXADDRESSES = 1;

    // Packet layout configuration. The nrf51822 has a highly capable and flexible RADIO module that, in addition to transmission
    // and reception of data, also contains a LENGTH field, two optional additional 1 byte fields (S0 and S1) and a CRC calculation.
    // Configure the packet format for a simple 8 bit length field and no additional fields.
    NRF_RADIO->PCNF0 = 0x00000008;
    // NRF_RADIO->PCNF1 = 0x02040000 | MICROBIT_RADIO_MAX_PACKET_SIZE;
    // NRF_RADIO->PCNF1 = 0x00040000 | MICROBIT_RADIO_MAX_PACKET_SIZE;
    // 14 bytes of frame plus 32 byte payload.
    NRF_RADIO->PCNF1 = 0x000E0000 | MICROBIT_RADIO_MAX_PACKET_SIZE;


    // Most communication channels contain some form of checksum - a mathematical calculation taken based on all the data
    // in a packet, that is also sent as part of the packet. When received, this calculation can be repeated, and the results
    // from the sender and receiver compared. If they are different, then some corruption of the data ahas happened in transit,
    // and we know we can't trust it. The nrf51822 RADIO uses a CRC for this - a very effective checksum calculation.
    //
    // Enable automatic 16bit CRC generation and checking, and configure how the CRC is calculated.
    NRF_RADIO->CRCCNF = RADIO_CRCCNF_LEN_Two;
    NRF_RADIO->CRCINIT = 0xFFFF;
    NRF_RADIO->CRCPOLY = 0x11021;

    // Set the start random value of the data whitening algorithm. This can be any non zero number.
    NRF_RADIO->DATAWHITEIV = 0x18;

    // Set up the RADIO module to read and write from our internal buffer.
    NRF_RADIO->PACKETPTR = (uint32_t)rxBuf;

    // Configure the hardware to issue an interrupt whenever a task is complete (e.g. send/receive).
    NRF_RADIO->INTENSET = 0x00000008;
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_SetPriority(RADIO_IRQn, 1);
    NVIC_EnableIRQ(RADIO_IRQn);

    //NRF_RADIO->SHORTS |= RADIO_SHORTS_ADDRESS_RSSISTART_Msk;

    // Start listening for the next packet
    radio_status |= RADIO_STATUS_RECV_EN;
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_RXEN = 1;
    while(NRF_RADIO->EVENTS_READY == 0);

    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->TASKS_START = 1;

    // register ourselves for a callback event, in order to empty the receive queue.
    fiber_add_idle_component(this);

    // Done. Record that our RADIO is configured.
    status |= MICROBIT_RADIO_STATUS_INITIALISED;

    return MICROBIT_OK;
}

/**
  * Disables the radio for use as a multipoint sender/receiver.
  *
  * @return MICROBIT_OK on success, MICROBIT_NOT_SUPPORTED if the BLE stack is running.
  */
int MicroBitPeridoRadio::disable()
{
    // Only attempt to enable.disable the radio if the protocol is alreayd running.
    if (ble_running())
        return MICROBIT_NOT_SUPPORTED;

    if (!(status & MICROBIT_RADIO_STATUS_INITIALISED))
        return MICROBIT_OK;

    // Disable interrupts and STOP any ongoing packet reception.
    NVIC_DisableIRQ(RADIO_IRQn);

    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while(NRF_RADIO->EVENTS_DISABLED == 0);

    // deregister ourselves from the callback event used to empty the receive queue.
    fiber_remove_idle_component(this);

    // record that the radio is now disabled
    status &= ~MICROBIT_RADIO_STATUS_INITIALISED;

    return MICROBIT_OK;
}

/**
  * Sets the radio to listen to packets sent with the given group id.
  *
  * @param group The group to join. A micro:bit can only listen to one group ID at any time.
  *
  * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the BLE stack is running.
  */
int MicroBitPeridoRadio::setGroup(uint8_t group)
{
    if (ble_running())
        return MICROBIT_NOT_SUPPORTED;

    // Record our group id locally
    this->group = group;

    // Also append it to the address of this device, to allow the RADIO module to filter for us.
    NRF_RADIO->PREFIX0 = (uint32_t)group;

    return MICROBIT_OK;
}

/**
  * Set the current period in milliseconds broadcasted in the perido frame
  *
  * @param period_ms the new period, in milliseconds.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the period is too short.
  */
int MicroBitPeridoRadio::setPeriod(uint32_t period_ms)
{
    if(period_ms < 10)
        return MICROBIT_INVALID_PARAMETER;

    sleepPeriodMs = period_ms;

    return MICROBIT_OK;
}

/**
  * Retrieve the current period in milliseconds broadcasted in the perido frame
  *
  * @return the current period in milliseconds
  */
uint32_t MicroBitPeridoRadio::getPeriod()
{
    return sleepPeriodMs;
}

/**
  * A background, low priority callback that is triggered whenever the processor is idle.
  * Here, we empty our queue of received packets, and pass them onto higher level protocol handlers.
  */
void MicroBitPeridoRadio::idleTick()
{
    // Walk the list of packets and process each one.
    while(rxQueue)
    {
        PeridoFrameBuffer *p = rxQueue;

        MicroBitEvent(MICROBIT_ID_RADIO_DATA_READY, p->protocol);

        // If the packet was processed, it will have been recv'd, and taken from the queue.
        // If this was a packet for an unknown protocol, it will still be there, so simply free it.
        if (p == rxQueue)
        {
            recv();
            delete p;
        }
    }
}

/**
  * Determines the number of packets ready to be processed.
  *
  * @return The number of packets in the receive buffer.
  */
int MicroBitPeridoRadio::dataReady()
{
    return queueDepth;
}

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
PeridoFrameBuffer* MicroBitPeridoRadio::recv()
{
    PeridoFrameBuffer *p = rxQueue;

    if (p)
    {
         // Protect shared resource from ISR activity
        NVIC_DisableIRQ(RADIO_IRQn);

        rxQueue = rxQueue->next;
        queueDepth--;

        // Allow ISR access to shared resource
        NVIC_EnableIRQ(RADIO_IRQn);
    }

    return p;
}

/**
  * Transmits the given buffer onto the broadcast radio.
  * The call will wait until the transmission of the packet has completed before returning.
  *
  * @param data The packet contents to transmit.
  *
  * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the BLE stack is running.
  */
int MicroBitPeridoRadio::send(PeridoFrameBuffer *buffer)
{
    __disable_irq();
    set_gpio(1);

    // // // Turn off the transceiver.
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    for (volatile int i =0; i < 131; i++);

    radio_status = RADIO_STATUS_DISABLED;

    // Configure the radio to send the buffer provided.
    NRF_RADIO->PACKETPTR = (uint32_t) buffer;

    // Turn on the transmitter, and wait for it to signal that it's ready to use.
    radio_status = RADIO_STATUS_TX_EN;
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_TXEN = 1;
    for (volatile int i =0; i < 522; i++);
    radio_status = RADIO_STATUS_TX_RDY;

    set_gpio(0);

    set_gpio(1);
    radio_status = RADIO_STATUS_TX_ST;
    // Start transmission and wait for end of packet.
    NRF_RADIO->TASKS_START = 1;
    NRF_RADIO->EVENTS_END = 0;
    while(NRF_RADIO->EVENTS_END == 0);
    radio_status = RADIO_STATUS_TX_END;
    set_gpio(0);

    set_gpio(1);
    NRF_RADIO->PACKETPTR = (uint32_t) rxBuf;
    // Turn off the transmitter.
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while(NRF_RADIO->EVENTS_DISABLED == 0);
    radio_status = RADIO_STATUS_DISABLED;

    // Start listening for the next packet
    radio_status = RADIO_STATUS_RECV_EN;
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_RXEN = 1;
    while(NRF_RADIO->EVENTS_READY == 0);

    radio_status = RADIO_STATUS_RECV_RDY;

    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_END = 0;
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    set_gpio(0);
    // Re-enable the Radio interrupt.
    __enable_irq();
    NRF_RADIO->TASKS_START = 1;

    radio_status |= RADIO_STATUS_RETRANS;

    return MICROBIT_OK;
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
int MicroBitPeridoRadio::send(uint8_t *buffer, int len)
{
    if (buffer == NULL || len < 0 || len > MICROBIT_RADIO_MAX_PACKET_SIZE + MICROBIT_PERIDO_HEADER_SIZE - 1)
        return MICROBIT_INVALID_PARAMETER;

    PeridoFrameBuffer buf;

    buf.length = len + MICROBIT_PERIDO_HEADER_SIZE - 1;
    buf.version = 1;
    buf.group = 0;
    buf.protocol = MICROBIT_RADIO_PROTOCOL_PERIDO;
    buf.ttl = 4;
    buf.sleep_period_ms = getPeriod();
    microbit_seed_random();
    buf.id = microbit_random(0x7FFFFFFF);
    memcpy(buf.payload, buffer, len);

    return send(&buf);
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
int MicroBitPeridoRadio::send(PacketBuffer data)
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
int MicroBitPeridoRadio::send(ManagedString data)
{
    return send((uint8_t *)data.toCharArray(), data.length());
}