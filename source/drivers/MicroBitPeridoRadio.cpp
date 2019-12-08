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
#include "MicroBitComponent.h"
#include "EventModel.h"
#include "MicroBitDevice.h"
#include "ErrorNo.h"
#include "MicroBitFiber.h"
#include "MicroBitBLEManager.h"
#include "MicroBitHeapAllocator.h"

extern void inline
__attribute__((__gnu_inline__,__always_inline__))
accurate_delay_us(uint32_t volatile number_of_us)
{
    __ASM volatile (
        "1:\tSUB %0, %0, #1\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "BNE 1b\n\t"
        : "+r"(number_of_us)
    );
}

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

/**
 *  Timings for each event (us):
 *
 *  TX Enable               135
 *  TX (15 bytes)           166
 *  DISABLE                 10
 *  RX Enable               135
 **/
#define DISCOVERY_TX_BACKOFF_TIME   40000
#define DISCOVERY_BACKOFF_TIME      (DISCOVERY_TX_BACKOFF_TIME * 2)

#define DISCOVERY_TX_BACKOFF_TIME_RUNNING   40000

#define TX_BACKOFF_MIN              200
#define TX_BACKOFF_TIME             (3000 - TX_BACKOFF_MIN)
#define TX_TIME                     300         // includes tx time for a larger packet
#define TX_ENABLE_TIME              350         // includes processing time overhead on reception for other nodes...
#define RX_ENABLE_TIME              200
#define RX_TX_DISABLE_TIME          30          // 10 us is pretty pointless for a timer callback.
#define TX_ADDRESS_TIME             64

#define TIME_TO_TRANSMIT_ADDR       RX_TX_DISABLE_TIME + TX_ENABLE_TIME + TX_ADDRESS_TIME

#define FORWARD_POLL_TIME           2500
#define ABSOLUTE_RESPONSE_TIME      10000
#define PERIDO_DEFAULT_PERIOD_IDX   2

#define TIME_TO_TRANSMIT_BYTE_1MB   8

#define NO_RESPONSE_THRESHOLD       5
#define LAST_SEEN_BUFFER_SIZE       10
#define OUT_TIME_BUFFER_SIZE        6

#define DISCOVERY_PACKET_THRESHOLD  TX_BACKOFF_TIME + TX_BACKOFF_MIN
#define DISCOVERY_TIME_ARRAY_LEN    3

#define PERIDO_WAKE_THRESHOLD_MAX   1000
#define PERIDO_WAKE_THRESHOLD_MID   500

#define PERIDO_WAKE_TOLERANCE       30

#define CONSTANT_SYNC_OFFSET        110

#define WAKE_UP_CHANNEL             0
#define GO_TO_SLEEP_CHANNEL         1
#define CHECK_TX_CHANNEL            2
#define STATE_MACHINE_CHANNEL       3

#define PERIOD_COUNT                13

#define SPEED_THRESHOLD_MAX         5
#define SPEED_THRESHOLD_MIN         -5

#define TX_PACKETS_SIZE             (2 * MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS)

uint16_t periods[PERIOD_COUNT] = {10, 20, 40, 80, 160, 320, 640, 1280, 2560, 5120, 10240, 20480, 40960};

// volatile uint32_t radio_status = 0;
volatile uint32_t no_response_count = 0;

volatile uint32_t discovery_tx_time = DISCOVERY_TX_BACKOFF_TIME;

volatile int8_t speed = 0;
volatile static uint8_t network_period_idx = PERIDO_DEFAULT_PERIOD_IDX;

volatile uint32_t crc_fail_count = 0;
volatile uint32_t packet_count = 0;
volatile uint32_t retransmit_count = 0;

volatile uint32_t packets_received = 0;
volatile uint32_t packets_transmitted = 0;
volatile uint32_t packets_forwarded = 0;

volatile static uint32_t current_cc = 0;
volatile static uint32_t period_start_cc = 0;
volatile static uint32_t correction = 0;

volatile uint8_t last_seen_index = 0;
volatile uint32_t last_seen[LAST_SEEN_BUFFER_SIZE] = { 0 };

volatile uint8_t  tx_packets_head = 0;
volatile uint8_t  tx_packets_tail = 0;
volatile uint32_t tx_packets[TX_PACKETS_SIZE] = { 0 };

// we maintain the number of wakes where we haven't seen a transmission, if we hit the match variable,
//we queue a keep alive packet
volatile static uint8_t keep_alive_count = 0;
volatile static uint8_t keep_alive_match = 0;

#define RADIO_STATE_RECEIVE  (1)
#define RADIO_STATE_TRANSMIT  (2)
#define RADIO_STATE_FORWARD  (3)

volatile uint8_t radioState = RADIO_STATE_RECEIVE;

void timer_callback(uint8_t) {}


extern void set_transmission_reception_gpio(int);

void poop()
{
    volatile int i = 0;

    i++;
    return;
}

extern "C" void RADIO_IRQHandler(void)
{
    if (NRF_RADIO->EVENTS_END)
    {
        NRF_RADIO->EVENTS_END = 0;
        PeridoFrameBuffer *p = MicroBitPeridoRadio::instance->rxBuf;

        if (radioState == RADIO_STATE_RECEIVE)
        {
            if(NRF_RADIO->CRCSTATUS == 1)
            {
                if(p->ttl > 0)
                {
                    p->ttl--;
                    NRF_RADIO->PACKETPTR = (uint32_t)p;
                    NRF_RADIO->TASKS_TXEN = 1;
                    radioState = RADIO_STATE_FORWARD;
                    // swap to forward mode
                    // PERIDO_UNSET_FLAGS(RADIO_STATUS_RX_RDY | RADIO_STATUS_RECEIVE);
                    // policy decisions could be implemented here. i.e. forward only ours, forward all, whitelist
                    // PERIDO_SET_FLAGS((RADIO_STATUS_FORWARD | RADIO_STATUS_DISABLE | RADIO_STATUS_TX_EN));
                    // PERIDO_UNSET_FLAGS(RADIO_STATUS_RECEIVE);
                    // PERIDO_SET_FLAGS(RADIO_STATUS_FORWARD);
    // #ifdef TRACE
    //                     set_transmission_reception_gpio(1);
    // #endif
                    // HW_ASSERT(0,11);
                    packets_received++;
                    return;
                }
            }

            packets_received++;
            crc_fail_count++;

            // HW_ASSERT(0,11);
            NRF_RADIO->PACKETPTR = (uint32_t)p;
            NRF_RADIO->TASKS_RXEN = 1;
            return;
        }

        if (radioState == RADIO_STATE_TRANSMIT)
        {
            // #ifdef TRACE
                // set_transmission_reception_gpio(0);
    // #endif
                // HW_ASSERT(0,11);
                NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->rxBuf;
                NRF_RADIO->TASKS_RXEN = 1;
                packets_transmitted++;
                radioState = RADIO_STATE_RECEIVE;
    // #ifdef TRACE
    //             set_transmission_reception_gpio(1);
    // #endif
            return;
        }

        if (radioState == RADIO_STATE_FORWARD)
        {
            // #ifdef TRACE
    //             set_transmission_reception_gpio(0);
    // #endif
                // HW_ASSERT(0,11);
                NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->rxBuf;
                NRF_RADIO->TASKS_RXEN = 1;
                radioState = RADIO_STATE_RECEIVE;
                packets_forwarded++;
            return;
    // #ifdef TRACE
    //             set_transmission_reception_gpio(1);
    // #endif
        }
    }
}

//     switch (radioState) {
//         case Receive:
// // #ifdef TRACE
// //             set_transmission_reception_gpio(0);
// // #endif
//             if(NRF_RADIO->CRCSTATUS == 1)
//             {
//                 if(p->ttl > 0)
//                 {
//                     p->ttl--;
//                     NRF_RADIO->PACKETPTR = (uint32_t)p;
//                     NRF_RADIO->TASKS_TXEN = 1;
//                     radioState = Forward;
//                     // swap to forward mode
//                     // PERIDO_UNSET_FLAGS(RADIO_STATUS_RX_RDY | RADIO_STATUS_RECEIVE);
//                     // policy decisions could be implemented here. i.e. forward only ours, forward all, whitelist
//                     // PERIDO_SET_FLAGS((RADIO_STATUS_FORWARD | RADIO_STATUS_DISABLE | RADIO_STATUS_TX_EN));
//                     // PERIDO_UNSET_FLAGS(RADIO_STATUS_RECEIVE);
//                     // PERIDO_SET_FLAGS(RADIO_STATUS_FORWARD);
// // #ifdef TRACE
// //                     set_transmission_reception_gpio(1);
// // #endif
//                     // HW_ASSERT(0,11);
//                     packets_received++;
//                     break;
//                 }
//             }

//             packets_received++;
//             crc_fail_count++;

//             // HW_ASSERT(0,11);
//             NRF_RADIO->PACKETPTR = (uint32_t)p;
//             NRF_RADIO->TASKS_RXEN = 1;
// // #ifdef TRACE
// //             set_transmission_reception_gpio(1);
// // #endif
//             break;
//         case Transmit:
// // #ifdef TRACE
//             // set_transmission_reception_gpio(0);
// // #endif
//             // HW_ASSERT(0,11);
//             NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->rxBuf;
//             NRF_RADIO->TASKS_RXEN = 1;
//             packets_transmitted++;
//             radioState = Receive;
// // #ifdef TRACE
// //             set_transmission_reception_gpio(1);
// // #endif
//             break;

//         case Forward:

// // #ifdef TRACE
// //             set_transmission_reception_gpio(0);
// // #endif
//             // HW_ASSERT(0,11);
//             NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->rxBuf;
//             NRF_RADIO->TASKS_RXEN = 1;
//             radioState = Receive;
//             packets_forwarded++;
// // #ifdef TRACE
// //             set_transmission_reception_gpio(1);
// // #endif
//             break;
//     }
// }


//         if (radio_status & RADIO_STATUS_RECEIVE)
//         {
//             // PERIDO_ASSERT(!(radio_status & (RADIO_STATUS_FORWARD | RADIO_STATUS_TRANSMIT)))
//             packets_received++;
// #ifdef TRACE
//             set_transmission_reception_gpio(0);
// #endif
//             PeridoFrameBuffer *p = MicroBitPeridoRadio::instance->rxBuf;
//             if(NRF_RADIO->CRCSTATUS == 1)
//             {
//                 if(p->ttl > 0)
//                 {
//                     p->ttl--;
//                     radioState = Transmit;
//                     // swap to forward mode
//                     // PERIDO_UNSET_FLAGS(RADIO_STATUS_RX_RDY | RADIO_STATUS_RECEIVE);
//                     // policy decisions could be implemented here. i.e. forward only ours, forward all, whitelist
//                     // PERIDO_SET_FLAGS((RADIO_STATUS_FORWARD | RADIO_STATUS_DISABLE | RADIO_STATUS_TX_EN));
//                     // PERIDO_UNSET_FLAGS(RADIO_STATUS_RECEIVE);
//                     // PERIDO_SET_FLAGS(RADIO_STATUS_FORWARD);
// #ifdef TRACE
//                     set_transmission_reception_gpio(1);
// #endif
//                     // HW_ASSERT(0,11);
//                     NRF_RADIO->PACKETPTR = (uint32_t)p;
//                     NRF_RADIO->TASKS_TXEN = 1;
//                     return;
//                 }
//             }
//             else
//                 crc_fail_count++;

//             // HW_ASSERT(0,11);
//             NRF_RADIO->PACKETPTR = (uint32_t)p;
//             NRF_RADIO->TASKS_RXEN = 1;
// #ifdef TRACE
//             set_transmission_reception_gpio(1);
// #endif
//             return;
//         }

//         if (radio_status & (RADIO_STATUS_FORWARD))
//         {
//             // PERIDO_ASSERT(!(radio_status & (RADIO_STATUS_RECEIVE | RADIO_STATUS_TRANSMIT)))
//             packets_forwarded++;
// #ifdef TRACE
//             set_transmission_reception_gpio(0);
// #endif
//             PERIDO_UNSET_FLAGS(RADIO_STATUS_FORWARD);
//             PERIDO_SET_FLAGS(RADIO_STATUS_RECEIVE);

//             // HW_ASSERT(0,11);
//             NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->rxBuf;
//             NRF_RADIO->TASKS_RXEN = 1;
// #ifdef TRACE
//             set_transmission_reception_gpio(1);
// #endif
//             return;
//         }

//         if (radio_status & RADIO_STATUS_TRANSMIT)
//         {
//             // PERIDO_ASSERT(!(radio_status & (RADIO_STATUS_RECEIVE | RADIO_STATUS_FORWARD)))
// #ifdef TRACE
//             set_transmission_reception_gpio(0);
// #endif
//             packets_transmitted++;
//             PERIDO_UNSET_FLAGS(RADIO_STATUS_TRANSMIT);
//             PERIDO_SET_FLAGS(RADIO_STATUS_RECEIVE);

//             // HW_ASSERT(0,11);
//             NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->rxBuf;
//             NRF_RADIO->TASKS_RXEN = 1;
// #ifdef TRACE
//             set_transmission_reception_gpio(1);
// #endif
//             return;
//         }
    // }

/**
* automatic transposition of textual name to app id and namespace...
    * should there be a default app_id and namespace?
* The ability to create a "private" group where there aren't other devices is desired
    * could probably use the app_id to select a band...
    * it's on the user to create reliability.
    * Probably want some group collision detection and the selection of another group...
    * there are only 100 bands...
* increasing and decreasing data rates...
    * how do we reach concensus?
        * do we need a broadcast frame?
            * i'd rather not...
        * expresses of intent in a frame
            * only works if others have data to transmit.
        * First to broadcast, if others are ok with that then then they move, otherwise they stay where they are.
    * automatic slowdown on no transmission in wake period... inverse as well if we're not sleeping as often as we should, speed up.
**/

void manual_poke()
{
    if (MicroBitPeridoRadio::instance->getCurrentTxBuf() == NULL)
        return;

    NRF_RADIO->TASKS_DISABLE = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0);

    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->getCurrentTxBuf();
    // HW_ASSERT(0,11);
    NRF_RADIO->TASKS_TXEN = 1;
// #ifdef TRACE
//     set_transmission_reception_gpio(1);
// #endif
    radioState = RADIO_STATE_TRANSMIT;
    // radio_status = 0;
    // PERIDO_SET_FLAGS(RADIO_STATUS_TRANSMIT);
}

/**
  * Constructor.
  *
  * Initialise the MicroBitPeridoRadio.
  *
  * @note This class is demand activated, as a result most resources are only
  *       committed if send/recv or event registrations calls are made.
  */
MicroBitPeridoRadio::MicroBitPeridoRadio(LowLevelTimer& timer, uint8_t appId, uint16_t id) : timer(timer), cloud(*this, MICROBIT_PERIDO_CLOUD_NAMESPACE), datagram(*this, MICROBIT_PERIDO_DATAGRAM_NAMESPACE), event(*this, MICROBIT_PERIDO_EVENT_NAMESPACE)
{
    this->id = id;
    this->appId = appId;
    this->status = 0;
    this->rxQueueDepth = 0;
    this->txQueueDepth = 0;

    this->rxBuf = NULL;

    memset(this->rxArray, 0, sizeof(PeridoFrameBuffer*) * MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS);
    this->rxHead = 0;
    this->rxTail = 0;

    memset(this->txArray, 0, sizeof(PeridoFrameBuffer*) * MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS);
    this->txHead = 0;
    this->txTail = 0;

    timer.disable();

    timer.setIRQ(timer_callback);

    // timer mode
    timer.setMode(TimerModeTimer);

    // 32-bit
    timer.setBitMode(BitMode32);

    // 16 Mhz / 2^4 = 1 Mhz
    timer.setPrescaler(4);

    timer.enable();

    microbit_seed_random();

    this->periodIndex = PERIDO_DEFAULT_PERIOD_IDX;

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
int MicroBitPeridoRadio::copyRxBuf()
{
    if (rxBuf == NULL)
        return MICROBIT_INVALID_PARAMETER;

    uint8_t nextTail = (this->rxTail + 1) % MICROBIT_RADIO_MAXIMUM_RX_BUFFERS;

    if (nextTail == this->rxHead)
        return MICROBIT_NO_RESOURCES;

    // Ensure that a replacement buffer is available before queuing.
    PeridoFrameBuffer *newRxBuf = new PeridoFrameBuffer();

    if (newRxBuf == NULL)
        return MICROBIT_NO_RESOURCES;

    memcpy(newRxBuf, rxBuf, sizeof(PeridoFrameBuffer));

    // add our buffer to the array before updating the head
    // this ensures atomicity.
    this->rxArray[nextTail] = newRxBuf;
    this->rxTail = nextTail;

    // Increase our received packet count
    rxQueueDepth++;

    return MICROBIT_OK;
}

/**
  * Retrieve a pointer to the currently allocated receive buffer. This is the area of memory
  * actively being used by the radio hardware to store incoming data.
  *
  * @return a pointer to the current receive buffer.
  */
int MicroBitPeridoRadio::popTxQueue()
{
    if (this->txTail == this->txHead)
        return MICROBIT_OK;

    uint8_t nextHead = (this->txHead + 1) % MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS;
    PeridoFrameBuffer *p = txArray[nextHead];
    this->txArray[nextHead] = NULL;
    this->txHead = nextHead;
    txQueueDepth--;

    tx_packets[tx_packets_tail] = (p->namespace_id << 16) | p->id;

    uint8_t next_tx_tail = (tx_packets_tail + 1) % TX_PACKETS_SIZE;
    if (next_tx_tail != tx_packets_head)
        tx_packets_tail = next_tx_tail;

    delete p;

    return MICROBIT_OK;
}

PeridoFrameBuffer* MicroBitPeridoRadio::getCurrentTxBuf()
{
    if (this->txTail == this->txHead)
        return NULL;

    uint8_t nextTx = (this->txHead + 1) % MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS;
    return this->txArray[nextTx];
}

int MicroBitPeridoRadio::queueTxBuf(PeridoFrameBuffer* tx)
{
    uint8_t nextTail = (this->txTail + 1) % MICROBIT_PERIDO_MAXIMUM_TX_BUFFERS;

    if (nextTail == this->txHead)
        return MICROBIT_NO_RESOURCES;

    // Ensure that a replacement buffer is available before queuing.
    PeridoFrameBuffer *newTx = new PeridoFrameBuffer();

    if (newTx == NULL)
        return MICROBIT_NO_RESOURCES;

    memcpy(newTx, tx, sizeof(PeridoFrameBuffer));

    // add our buffer to the array before updating the head
    // this ensures atomicity.
    this->txArray[nextTail] = newTx;
    __disable_irq();
    this->txTail = nextTail;
    __enable_irq();

    txQueueDepth++;

    return MICROBIT_OK;
}

int MicroBitPeridoRadio::queueKeepAlive()
{
    // PeridoFrameBuffer buf;

    // buf.id = microbit_random(65535);
    // buf.length = 0 + MICROBIT_PERIDO_HEADER_SIZE - 1; // keep alive has no content.
    // buf.app_id = appId;
    // buf.namespace_id = 0;
    // buf.flags |= MICROBIT_PERIDO_FRAME_KEEP_ALIVE_FLAG;
    // buf.ttl = 2;
    // buf.initial_ttl = 2;
    // buf.time_since_wake = 0;
    // buf.period = 0;

    // return queueTxBuf(&buf);
    return MICROBIT_OK;
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

    // If this is the first time we've been enable, allocate our receive buffers.
    if (rxBuf == NULL)
        rxBuf = new PeridoFrameBuffer();

    if (rxBuf == NULL)
        return MICROBIT_NO_RESOURCES;

    keep_alive_count = 0;
    keep_alive_match = 0;

    while (keep_alive_match < 11)
        keep_alive_match = microbit_random(256) % 40;

    // Enable the High Frequency clock on the processor. This is a pre-requisite for
    // the RADIO module. Without this clock, no communication is possible.
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    NRF_RADIO->POWER = 0;
    NRF_RADIO->POWER = 1;

    NRF_RADIO->TXPOWER = (uint32_t)MICROBIT_BLE_POWER_LEVEL[6];
    NRF_RADIO->FREQUENCY = 2;

    // Bring up the nrf51822 RADIO module in Nordic's proprietary 1MBps packet radio mode.
    // setTransmitPower(MICROBIT_RADIO_DEFAULT_TX_POWER);

    // setFrequencyBand(MICROBIT_RADIO_DEFAULT_FREQUENCY);
    NRF_RADIO->OVERRIDE0 = NRF_FICR->BLE_1MBIT[0];
    NRF_RADIO->OVERRIDE1 = NRF_FICR->BLE_1MBIT[1];
    NRF_RADIO->OVERRIDE2 = NRF_FICR->BLE_1MBIT[2];
    NRF_RADIO->OVERRIDE3 = NRF_FICR->BLE_1MBIT[3];
    NRF_RADIO->OVERRIDE4 = 0x80000000 | NRF_FICR->BLE_1MBIT[4];

    // Configure for 1Mbps throughput.
    // This may sound excessive, but running a high data rates reduces the chances of collisions...
    NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit;

    // Configure the addresses we use for this protocol. We run ANONYMOUSLY at the core.
    // A 40 bit addresses is used. The first 32 bits match the ASCII character code for "uBit".
    // Statistically, this provides assurance to avoid other similar 2.4GHz protocols that may be in the vicinity.
    // We also map the assigned 8-bit GROUP id into the PREFIX field. This allows the RADIO hardware to perform
    // address matching for us, and only generate an interrupt when a packet matching our group is received.
    NRF_RADIO->BASE0 =  MICROBIT_RADIO_BASE_ADDRESS;

    NRF_RADIO->PREFIX0 = 0;

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

    NRF_RADIO->PCNF1 = 0x02040000 | MICROBIT_PERIDO_MAX_PACKET_SIZE;

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
    NRF_RADIO->DATAWHITEIV = 37;

    // Set up the RADIO module to read and write from our internal buffer.
    NRF_RADIO->PACKETPTR = (uint32_t)rxBuf;

    // Configure the hardware to issue an interrupt whenever a task is complete (e.g. send/receive)
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_SetPriority(RADIO_IRQn, 0);
    NVIC_EnableIRQ(RADIO_IRQn);

    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->INTENCLR = 0xffffffff;
    NRF_RADIO->INTENSET = 0x8;
    // NRF_RADIO->TIFS = 148;
    NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->rxBuf;
    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk; //| RADIO_SHORTS_ADDRESS_RSSISTART_Msk;

    radioState = RADIO_STATE_RECEIVE;
    // radio_status = 0;
    // PERIDO_SET_FLAGS(RADIO_STATUS_RECEIVE);
    NRF_RADIO->TASKS_RXEN = 1;

    // PERIDO_SET_FLAGS(RADIO_STATUS_DISABLED | RADIO_STATUS_DISCOVERING | RADIO_STATUS_SLEEPING);
    // timer.setCompare(WAKE_UP_CHANNEL, timer.captureCounter(WAKE_UP_CHANNEL) + periods[periodIndex] * 1000);

    // Done. Record that our RADIO is configured.
    status |= MICROBIT_RADIO_STATUS_INITIALISED;

    fiber_add_idle_component(this);

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

    // record that the radio is now disabled
    status &= ~MICROBIT_RADIO_STATUS_INITIALISED;

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
    int index = -1;

    for (int i = 0 ; i < PERIOD_COUNT; i++)
    {
        if (periods[i] < period_ms)
            continue;

        index = i;
        break;
    }

    if (index == -1)
        index = PERIOD_COUNT - 1;

    periodIndex = index;

    return MICROBIT_OK;
}

/**
  * Retrieve the current period in milliseconds broadcasted in the perido frame
  *
  * @return the current period in milliseconds
  */
uint32_t MicroBitPeridoRadio::getPeriod()
{
    return periods[periodIndex];
}

int MicroBitPeridoRadio::setGroup(uint8_t id)
{
    return setAppId(id);
}

int MicroBitPeridoRadio::setAppId(uint16_t id)
{
    this->appId = id;
    return MICROBIT_OK;
}

int MicroBitPeridoRadio::getAppId()
{
    return this->appId;
}

/**
  * Determines the number of packets ready to be processed.
  *
  * @return The number of packets in the receive buffer.
  */
int MicroBitPeridoRadio::dataReady()
{
    return rxQueueDepth;
}

PeridoFrameBuffer* MicroBitPeridoRadio::peakRxQueue()
{
    if (this->rxTail == this->rxHead)
        return NULL;

    uint8_t nextHead = (this->rxHead + 1) % MICROBIT_RADIO_MAXIMUM_RX_BUFFERS;
    return rxArray[nextHead];
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
    if (this->rxTail == this->rxHead)
        return NULL;

    uint8_t nextHead = (this->rxHead + 1) % MICROBIT_RADIO_MAXIMUM_RX_BUFFERS;

    PeridoFrameBuffer *p = rxArray[nextHead];
    this->rxArray[nextHead] = NULL;
    this->rxHead = nextHead;
    rxQueueDepth--;

    return p;
}

void MicroBitPeridoRadio::idleTick()
{
    // if (radio_status & RADIO_STATUS_QUEUE_KEEP_ALIVE)
    // {
    //     queueKeepAlive();
    //     PERIDO_UNSET_FLAGS(RADIO_STATUS_QUEUE_KEEP_ALIVE);
    // }

    // walk the array of tx'd packets and fire packetTransmitted for each driver...
    while (tx_packets_head != tx_packets_tail)
    {
        uint8_t next_tx_head = (tx_packets_head + 1) % TX_PACKETS_SIZE;
        uint8_t namespace_id = tx_packets[tx_packets_head] >> 16;
        uint16_t id = tx_packets[tx_packets_head] & 0xFFFF;

        if (namespace_id == cloud.getNamespaceId())
            cloud.packetTransmitted(id);

        tx_packets_head = next_tx_head;
    }

    // Walk the list of received packets and process each one.
    PeridoFrameBuffer* p = NULL;
    while ((p = peakRxQueue()) != NULL)
    {
        if (p->namespace_id == cloud.getNamespaceId())
            cloud.packetReceived();

        else if (p->namespace_id == datagram.getNamespaceId())
            datagram.packetReceived();

        else if (p->namespace_id == event.getNamespaceId())
            event.packetReceived();

        else
            delete recv();
    }
}


/**
  * Transmits the given buffer onto the broadcast radio.
  * The call will wait until the transmission of the packet has completed before returning.
  *
  * @param data The packet contents to transmit.
  *
  * @return MICROBIT_OK on success, or MICROBIT_NOT_SUPPORTED if the BLE stack is running.
  */
int MicroBitPeridoRadio::send(PeridoFrameBuffer* buffer)
{
    return queueTxBuf(buffer);
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
int MicroBitPeridoRadio::send(uint8_t *buffer, int len, uint8_t namespaceId)
{
    if (buffer == NULL || len < 0 || len > MICROBIT_PERIDO_MAX_PACKET_SIZE + MICROBIT_PERIDO_HEADER_SIZE - 1)
        return MICROBIT_INVALID_PARAMETER;

    PeridoFrameBuffer buf;

    buf.id = microbit_random(65535);
    buf.length = len + MICROBIT_PERIDO_HEADER_SIZE - 1;
    buf.app_id = appId;
    buf.namespace_id = namespaceId;
    buf.ttl = 2;
    buf.initial_ttl = 2;
    buf.time_since_wake = 0;
    buf.period = 0;
    memcpy(buf.payload, buffer, len);

    return send(&buf);
}

uint16_t MicroBitPeridoRadio::generateId(uint8_t app_id, uint8_t namespace_id)
{
    uint16_t new_id;
    bool seenBefore = true;

    while (seenBefore)
    {
        seenBefore = false;
        new_id = microbit_random(65535);

        for (int i = 0; i < LAST_SEEN_BUFFER_SIZE; i++)
        {
            if (last_seen[i] > 0)
            {
                uint8_t seen_namespace_id = last_seen[i];
                uint8_t seen_app_id = last_seen[i] >> 8;

                // we can exit early here as if the namespaces don't match we're not interested.
                if (namespace_id != seen_namespace_id || app_id != seen_app_id)
                    continue;

                uint16_t packet_id = (last_seen[i] >> 16);
                if (packet_id == new_id)
                    seenBefore = true;
            }
        }
    }

    return new_id;
}

#endif
