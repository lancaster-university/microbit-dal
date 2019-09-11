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
// extern void log_string_ch(const char*);
// extern void log_num(int);

// #define LOG_STRING(str) (log_string_ch(str))
// #define LOG_NUM(str) (log_num(str))

#define LOG_STRING(...) ((void)0)
#define LOG_NUM(...) ((void)0)

#define PERIDO_ASSERT(cond) {\
                                        if (!(cond)) \
                                            microbit_panic(__LINE__);\
                                        }
volatile int hw_state = 0;
#define HW_ASSERT(expected_state, panic_num) {\
                                        hw_state = NRF_RADIO->STATE;\
                                        if (hw_state != expected_state) \
                                            microbit_panic(__LINE__);\
                                        }\

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

// begin static config options of perido:
#define FILTER // if def'd, the driver will filter packets based on app id
#ifdef FILTER
#warning filtering packets based on app id
#endif

#define DISABLE_SLEEP // if def'd the driver will never go to sleep.
#ifdef DISABLE_SLEEP
#warning sleep is disabled.
#endif

#define DISABLE_STORE // if def'd the driver will not store packets.
#ifdef DISABLE_STORE
#warning storing is disabled.
#endif


// #define TRACE_CRC_FAIL
#define TRACE_WAKE
#define TRACE_TX
#define TRACE

#ifdef TRACE
#warning TRACE is on...

extern void set_rx_gpio(int);
extern void set_tx_enable_gpio(int);
extern void set_transmit_gpio(int);
extern void set_fwd_gpio(int);
extern void set_crc_fail_gpio(int);
extern void set_store_disable_gpio(int);
extern void set_tx_callback_gpio(int);
extern void set_wake_gpio(int);
extern void set_transmission_reception_gpio(int);
extern void set_user_placed_gpio(int);


extern void packet_debug(PeridoFrameBuffer*);

extern void process_packet(PeridoFrameBuffer*);
extern void packet_missed(PeridoFrameBuffer *p);

extern void valid_packet_received(PeridoFrameBuffer*);
extern void increment_counter(int);

extern void log_string(const char * s);
extern void log_num(int num);
#endif

// low level states
#define LOW_LEVEL_STATE_MASK        0x000000FF      // a mask for removing or retaining low level state

#define RADIO_STATUS_RX_EN          0x00000001      // reception is enabled
#define RADIO_STATUS_RX_RDY         0x00000002      // available to receive packets

#define RADIO_STATUS_DISABLE        0x00000004      // the radio should be disabled
#define RADIO_STATUS_DISABLED       0x00000008      // the radio is disabled

#define RADIO_STATUS_TX_EN          0x00000010      // transmission is enabled
#define RADIO_STATUS_TX_RDY         0x00000020      // transmission is ready
#define RADIO_STATUS_TX_ST          0x00000040      // transmission has begun
#define RADIO_STATUS_TX_END         0x00000080      // transmission has finished

// high level actions
#define HIGH_LEVEL_STATE_MASK               0x000FFF00      // a mask for removing or retaining high level state

#define RADIO_STATUS_TRANSMIT               0x00000100      // moving into transmit mode to send a packet.
#define RADIO_STATUS_RECEIVE                0x00000200
#define RADIO_STATUS_FORWARD                0x00000400      // actively forwarding any received packets
#define RADIO_STATUS_RECEIVING              0x00000800      // in the act of currently receiving a packet.
#define RADIO_STATUS_STORE                  0x00001000      // indicates the storage of the rx'd packet is required.
#define RADIO_STATUS_DISCOVERING            0x00002000      // listening for packets after powering on, prevents sleeping in rx mode.
#define RADIO_STATUS_SLEEPING               0x00004000      // indicates that the window of transmission has passed, and we have entered sleep mode.
#define RADIO_STATUS_WAKE_CONFIGURED        0x00008000
#define RADIO_STATUS_EXPECT_RESPONSE        0x00010000
#define RADIO_STATUS_FIRST_PACKET           0x00020000
#define RADIO_STATUS_SAMPLING               0x00040000
#define RADIO_STATUS_QUEUE_KEEP_ALIVE       0x00080000

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

volatile uint32_t radio_status = 0;
volatile uint32_t no_response_count = 0;

volatile uint32_t discovery_tx_time = DISCOVERY_TX_BACKOFF_TIME;

volatile int8_t speed = 0;
volatile static uint8_t network_period_idx = PERIDO_DEFAULT_PERIOD_IDX;

volatile uint32_t crc_fail_count = 0;
volatile uint32_t packet_count = 0;

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


#define TRACK_STATE

#ifdef TRACK_STATE

// #define PHYS_STATE_SIZE         128
#define PHYS_STATE_SIZE         512

#warning TRACK_STATE_ON

uint32_t radio_state[PHYS_STATE_SIZE] = {0};
uint32_t radio_pointer = 0;

#define PERIDO_SET_FLAGS(flags) do {                        \
                            __disable_irq();       \
                            radio_status |= (flags);     \
                            radio_state[radio_pointer] = radio_status | (__LINE__ << 20) | 1 << 31;  \
                            __enable_irq();        \
                            radio_pointer = (radio_pointer + 1) % PHYS_STATE_SIZE;    \
                            }while(0)

#define PERIDO_UNSET_FLAGS(flags) do {                          \
                                __disable_irq();       \
                                radio_status &= ~(flags);    \
                                radio_state[radio_pointer] = radio_status | (__LINE__ << 20); \
                                __enable_irq();        \
                                radio_pointer = (radio_pointer + 1) % PHYS_STATE_SIZE;    \
                              }while(0)

#define PERIDO_LOG_FLAGS(flags) do {                          \
                                __disable_irq();       \
                                radio_state[radio_pointer] = flags | (__LINE__ << 20); \
                                __enable_irq();        \
                                radio_pointer = (radio_pointer + 1) % PHYS_STATE_SIZE;    \
                              }while(0)

#else

#define PERIDO_SET_FLAGS(flags) do {                        \
                                    radio_status |= flags;     \
                                }while(0)

#define PERIDO_UNSET_FLAGS(flags) do {                          \
                                    radio_status &= ~flags;    \
                                }while(0)


#define PERIDO_LOG_FLAGS(...) ((void)0)
#endif

uint32_t read_and_restart_wake()
{
    uint32_t t =  MicroBitPeridoRadio::instance->timer.captureCounter(WAKE_UP_CHANNEL);
    MicroBitPeridoRadio::instance->timer.setCompare(WAKE_UP_CHANNEL, current_cc);
    return t;
}

void radio_state_machine()
{
state_machine_start:

    if(radio_status & RADIO_STATUS_DISABLED)
    {
        PERIDO_LOG_FLAGS(radio_status);
#ifdef TRACE
#ifndef TRACE_TX
        set_tx_enable_gpio(1);
#endif
#endif
        HW_ASSERT(0, 22);
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->EVENTS_END = 0;
        NRF_RADIO->EVENTS_ADDRESS = 0;

        PERIDO_ASSERT(!(radio_status & RADIO_STATUS_TX_EN && radio_status & RADIO_STATUS_RX_EN));

        if(radio_status & RADIO_STATUS_TX_EN)
        {
            set_user_placed_gpio(1);
            PERIDO_ASSERT(!(radio_status & (RADIO_STATUS_RX_EN|RADIO_STATUS_RX_RDY|RADIO_STATUS_RECEIVING|RADIO_STATUS_RECEIVE)));
            // WE DON'T WANT THE ADDRESS EVENT
            NRF_RADIO->INTENCLR = 0x0000000A;
            NRF_RADIO->INTENSET = 0x00000008;
            PERIDO_UNSET_FLAGS(RADIO_STATUS_TX_EN | RADIO_STATUS_DISABLED);
            PERIDO_SET_FLAGS(RADIO_STATUS_TX_RDY);

            NRF_RADIO->EVENTS_READY = 0;
            NRF_RADIO->TASKS_TXEN = 1;

            // if (radio_status & RADIO_STATUS_FORWARD)
                MicroBitPeridoRadio::instance->timer.setCompare(STATE_MACHINE_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(STATE_MACHINE_CHANNEL) + TX_ENABLE_TIME + 200);
            // else
            //     MicroBitPeridoRadio::instance->timer.setCompare(STATE_MACHINE_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(STATE_MACHINE_CHANNEL) + TX_ENABLE_TIME);
#ifdef TRACE
#ifndef TRACE_TX
            set_tx_enable_gpio(0);
#endif
#endif
            return;
        }

        if(radio_status & RADIO_STATUS_RX_EN)
        {
            PERIDO_ASSERT(!(radio_status & (RADIO_STATUS_TX_EN|RADIO_STATUS_TX_RDY|RADIO_STATUS_TRANSMIT)));
            // WE WANT THE ADDRESS EVENT TO REDUCE COLLISIONS.
            NRF_RADIO->INTENCLR = 0x0000000A;
            NRF_RADIO->INTENSET = 0x0000000A;
            NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->rxBuf;

            PERIDO_UNSET_FLAGS(RADIO_STATUS_RX_EN | RADIO_STATUS_DISABLED);
            PERIDO_SET_FLAGS(RADIO_STATUS_RECEIVE);

            // takes 7 us to complete, not much point in a timer.
            NRF_RADIO->EVENTS_READY = 0;
            NRF_RADIO->TASKS_RXEN = 1;
            MicroBitPeridoRadio::instance->timer.setCompare(STATE_MACHINE_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(STATE_MACHINE_CHANNEL) + RX_ENABLE_TIME);
#ifdef TRACE
#ifndef TRACE_TX
            set_tx_enable_gpio(0);
#endif
#endif
            return;
        }
#ifdef TRACE
#ifndef TRACE_TX
        set_tx_enable_gpio(0);
#endif
#endif
        // we're disabled but haven't been configured for rx / tx DO NOT CONTINUE!
        PERIDO_ASSERT(0);
        return;
    }

    if(radio_status & RADIO_STATUS_RECEIVE)
    {
        PERIDO_LOG_FLAGS(radio_status);
        PERIDO_ASSERT(!(radio_status & (RADIO_STATUS_TX_EN|RADIO_STATUS_TX_RDY|RADIO_STATUS_TRANSMIT)));

        if (NRF_RADIO->EVENTS_READY)
        {
#ifdef TRACE
            set_rx_gpio(1);
#endif
            PERIDO_SET_FLAGS(RADIO_STATUS_RX_RDY);
            NRF_RADIO->EVENTS_READY = 0;
            NRF_RADIO->TASKS_START = 1;

#ifdef TRACE
            set_rx_gpio(0);
#endif

            return;
        }

        // we get an address event for rx, indicating we are in the process of receiving a packet. Update our status and return;
        if(NRF_RADIO->EVENTS_ADDRESS)
        {
#ifdef TRACE
            set_transmission_reception_gpio(1);
            set_rx_gpio(1);
#endif
            NRF_RADIO->EVENTS_ADDRESS = 0;
            PERIDO_SET_FLAGS(RADIO_STATUS_RECEIVING);

            // clear any timer cb's so we aren't interrupted in our critical section
            MicroBitPeridoRadio::instance->timer.captureCounter(GO_TO_SLEEP_CHANNEL);
            MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL);
            return;
        }

        if(NRF_RADIO->EVENTS_END)
        {
            packet_count++;

            PERIDO_UNSET_FLAGS(RADIO_STATUS_RECEIVING);

            NRF_RADIO->EVENTS_ADDRESS = 0;
            NRF_RADIO->EVENTS_END = 0;
            NRF_RADIO->TASKS_START = 1;

#ifdef TRACE
            set_transmission_reception_gpio(0);
            set_rx_gpio(0);
#endif

#ifndef DISABLE_SLEEP
            MicroBitPeridoRadio::instance->timer.setCompare(GO_TO_SLEEP_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(GO_TO_SLEEP_CHANNEL) + FORWARD_POLL_TIME);
#endif

            if(NRF_RADIO->CRCSTATUS == 1)
            {
                PeridoFrameBuffer *p = MicroBitPeridoRadio::instance->rxBuf;

                // catch ddos
                if (p->ttl > 4)
                    return;

                // reset our keep alive count, a transmission has been seen
                keep_alive_count = 0;

                // if we've been discovering and are now synced, it's
                // highly likely that our new wake sleep window
                // does not align with our previous tx callback check... cancel!
                // MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL);
                PERIDO_UNSET_FLAGS(RADIO_STATUS_DISCOVERING);

                if(p->ttl > 0)
                {
                    p->ttl--;
                    // swap to forward mode
                    PERIDO_UNSET_FLAGS(RADIO_STATUS_RX_RDY | RADIO_STATUS_RECEIVE);
                    // policy decisions could be implemented here. i.e. forward only ours, forward all, whitelist
                    PERIDO_SET_FLAGS((RADIO_STATUS_FORWARD | RADIO_STATUS_DISABLE | RADIO_STATUS_TX_EN));
                }
                else
                {
                    PERIDO_UNSET_FLAGS(RADIO_STATUS_FORWARD);

                    // store this packet (should be a nop in most cases as we store after every forward), then try and transmit...
                    MicroBitPeridoRadio::instance->timer.setCompare(CHECK_TX_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL) + TX_BACKOFF_MIN + microbit_random((periods[network_period_idx] / 4) * 2000));
                    PERIDO_SET_FLAGS(RADIO_STATUS_STORE);
                }
            }
            else
            {
                // check tx here
#ifdef TRACE
                set_crc_fail_gpio(1);
#endif
                MicroBitPeridoRadio::instance->timer.setCompare(CHECK_TX_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL) + FORWARD_POLL_TIME + microbit_random((periods[network_period_idx] / 4) * 2000));
                crc_fail_count++;
#ifdef TRACE
                set_crc_fail_gpio(0);
                set_rx_gpio(0);
#endif
                return;
            }
        }
#ifdef TRACE
        set_rx_gpio(0);
#endif
    }

    if(radio_status & RADIO_STATUS_TRANSMIT)
    {
        PERIDO_ASSERT(!(radio_status & RADIO_STATUS_FORWARD));
        PERIDO_LOG_FLAGS(radio_status);
#ifdef TRACE
        set_transmit_gpio(1);
#endif
        PERIDO_ASSERT(!(radio_status & (RADIO_STATUS_RX_EN|RADIO_STATUS_RX_RDY|RADIO_STATUS_RECEIVING|RADIO_STATUS_RECEIVE)));
        if (radio_status & RADIO_STATUS_TX_RDY)
        {
            set_user_placed_gpio(0);
            NRF_RADIO->EVENTS_READY = 0;

            HW_ASSERT(10, 33);

            MicroBitPeridoRadio::instance->timer.captureCounter(GO_TO_SLEEP_CHANNEL); // cancel sleep callback.
            PeridoFrameBuffer* p = MicroBitPeridoRadio::instance->getCurrentTxBuf();

            PERIDO_UNSET_FLAGS(RADIO_STATUS_TX_RDY);
            PERIDO_SET_FLAGS(RADIO_STATUS_TX_END);

            p->period = network_period_idx;
            p->flags = 0;
            p->ttl = p->initial_ttl;

            // if (radio_status & RADIO_STATUS_DISCOVERING)
            p->time_since_wake = 0;
            // else
            // {
            //     uint32_t t = read_and_restart_wake();

            //     // account for roll over... otherwise the network will be put to sleep for quite some time... :)
            //     if (period_start_cc < t)
            //         p->time_since_wake = t - period_start_cc;
            //     else
            //         p->time_since_wake = period_start_cc - t;
            // }

            NRF_RADIO->PACKETPTR = (uint32_t)p;
            // grab next packet and set pointer
#ifdef TRACE
            set_transmission_reception_gpio(1);
#ifdef TRACE_TX
            set_tx_enable_gpio(1);
#endif
#endif
            NRF_RADIO->TASKS_START = 1;
            NRF_RADIO->EVENTS_END = 0;

#ifdef TRACE
            set_transmit_gpio(0);
#endif
            return;
        }

        if(radio_status & RADIO_STATUS_TX_END)
        {
            NRF_RADIO->EVENTS_END = 0;
#ifdef TRACE
            set_transmission_reception_gpio(0);
#ifdef TRACE_TX
            set_tx_enable_gpio(0);
#endif
#endif
            PERIDO_UNSET_FLAGS(RADIO_STATUS_TX_END | RADIO_STATUS_TRANSMIT);
            PERIDO_SET_FLAGS((RADIO_STATUS_DISABLE | RADIO_STATUS_RX_EN | RADIO_STATUS_EXPECT_RESPONSE));

#ifndef DISABLE_SLEEP
            MicroBitPeridoRadio::instance->timer.setCompare(GO_TO_SLEEP_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(GO_TO_SLEEP_CHANNEL) + FORWARD_POLL_TIME);
#endif
        }
#ifdef TRACE
        set_transmit_gpio(0);
#endif
    }

    if (radio_status & RADIO_STATUS_FORWARD)
    {
        PERIDO_ASSERT(!(radio_status & RADIO_STATUS_TRANSMIT));
        PERIDO_LOG_FLAGS(radio_status);
        PERIDO_ASSERT(!(radio_status & (RADIO_STATUS_RX_EN|RADIO_STATUS_RX_RDY|RADIO_STATUS_RECEIVING|RADIO_STATUS_RECEIVE)));

#ifdef TRACE
        set_fwd_gpio(1);
#endif
        if(radio_status & RADIO_STATUS_TX_END)
        {
            NRF_RADIO->EVENTS_END = 0;
#ifdef TRACE
            set_transmission_reception_gpio(0);
#ifdef TRACE_TX
            set_tx_enable_gpio(0);
#endif
#endif
            PERIDO_UNSET_FLAGS(RADIO_STATUS_TX_END | RADIO_STATUS_FORWARD);
            PERIDO_SET_FLAGS(RADIO_STATUS_DISABLE | RADIO_STATUS_RX_EN);

            MicroBitPeridoRadio::instance->timer.setCompare(CHECK_TX_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL) + (MicroBitPeridoRadio::instance->rxBuf->ttl * FORWARD_POLL_TIME) + microbit_random((periods[network_period_idx] / 4) * 1000));
#ifndef DISABLE_SLEEP
            MicroBitPeridoRadio::instance->timer.setCompare(GO_TO_SLEEP_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(GO_TO_SLEEP_CHANNEL) + FORWARD_POLL_TIME);
#endif
        }

        if(radio_status & RADIO_STATUS_TX_RDY)
        {
            MicroBitPeridoRadio::instance->timer.captureCounter(GO_TO_SLEEP_CHANNEL); // cancel sleep callback.
            PERIDO_UNSET_FLAGS(RADIO_STATUS_TX_RDY);
            PERIDO_SET_FLAGS(RADIO_STATUS_TX_END);

            HW_ASSERT(10, 44);

            NRF_RADIO->PACKETPTR = (uint32_t)MicroBitPeridoRadio::instance->rxBuf;
#ifdef TRACE
            set_transmission_reception_gpio(1);
#ifdef TRACE_TX
            set_tx_enable_gpio(1);
#endif
#endif
            NRF_RADIO->TASKS_START = 1;
            NRF_RADIO->EVENTS_END = 0;

            PERIDO_SET_FLAGS(RADIO_STATUS_STORE);
        }
#ifdef TRACE
#ifndef TRACE_CRC_FAIL
        set_fwd_gpio(0);
#endif
#endif
    }

    if(radio_status & RADIO_STATUS_STORE)
    {
        PERIDO_LOG_FLAGS(radio_status);
#ifdef DISABLE_STORE
        // PeridoFrameBuffer* tx = MicroBitPeridoRadio::instance->getCurrentTxBuf();
#ifdef TRACE
        // if(radio_status & RADIO_STATUS_EXPECT_RESPONSE)
        // {
        //     process_packet(tx);
        // }
        // else
        // {
        //     process_packet(MicroBitPeridoRadio::instance->rxBuf);
        // }

#endif
        PERIDO_UNSET_FLAGS(RADIO_STATUS_STORE);
#else
#ifdef TRACE
        set_store_disable_gpio(1);
#endif
        PERIDO_UNSET_FLAGS(RADIO_STATUS_STORE);

        bool seen = false;
        PeridoFrameBuffer *p = MicroBitPeridoRadio::instance->rxBuf;
        uint32_t combined_id = (p->id << 16) | (p->app_id << 8) | p->namespace_id;

        // if we sent a packet, we also flagged that we expected a response.
        // If we don't see our own packet it means that there was a collision or we are out of sync.
        if(radio_status & RADIO_STATUS_EXPECT_RESPONSE)
        {
            PeridoFrameBuffer* tx = MicroBitPeridoRadio::instance->getCurrentTxBuf();

            bool app_id_same = tx->app_id == p->app_id;
            bool namespace_same = p->namespace_id == tx->namespace_id;
            bool id_same = p->id == tx->id;

            // if we get our own packet back pop our tx queue and reset our no_response_count
            if(app_id_same && namespace_same && id_same)
            {
#ifdef TRACE
                process_packet(tx);
#endif
                // only pop our tx buffer if something responds
                MicroBitPeridoRadio::instance->popTxQueue();
                last_seen[last_seen_index] = combined_id;
                last_seen_index = (last_seen_index + 1) %  LAST_SEEN_BUFFER_SIZE;

                // we received a response, reset our counter.
                no_response_count = 0;
                seen = true;
            }

            // we could increment no_response_count here, but it is done when we go to sleep.
            PERIDO_UNSET_FLAGS(RADIO_STATUS_EXPECT_RESPONSE);
        }

        // if this is the first packet we are storing, then calculate how far off the original senders period we are.
        // don't sync to a proposal frame
        if (radio_status & RADIO_STATUS_FIRST_PACKET /*&& !(p->flags & MICROBIT_PERIDO_FRAME_PROPOSAL_FLAG)*/)
        {
            PERIDO_UNSET_FLAGS(RADIO_STATUS_FIRST_PACKET);

            uint32_t t = p->time_since_wake;
            uint32_t period = periods[p->period] * 1000;
            uint8_t hops = (p->initial_ttl - p->ttl);

            // correct and set wake up period.
            correction = (t + (hops * ((p->length * TIME_TO_TRANSMIT_BYTE_1MB) + RX_TX_DISABLE_TIME + TX_ENABLE_TIME)));

            correction %= period;

// this will fire if someone sends a time_since_wake that is greater than the current period
#ifdef TRAP_PERIOD_EXTENSION
            if (p->time_since_wake > period || correction > period)
                while(1)
                {
                    #warning remove eventually
                    LOG_STRING("TSC: ");
                    LOG_NUM(p->time_since_wake);
                    LOG_STRING("HOPS: ");
                    LOG_NUM(hops);
                    LOG_STRING("LENGTH: ");
                    LOG_NUM(p->length);
                    LOG_STRING("CORRECTION: ");
                    LOG_NUM(correction);
                    microbit_panic(890);
                    wait_ms(1000);
                }
#endif

            // current_cc = MicroBitPeridoRadio::instance->timer.captureCounter(WAKE_UP_CHANNEL) + (period - correction);
            // MicroBitPeridoRadio::instance->timer.setCompare(WAKE_UP_CHANNEL, current_cc);
        }

        if (p->flags & MICROBIT_PERIDO_FRAME_KEEP_ALIVE_FLAG)
            seen = true;

        // check if we've seen this ID before...
        for (int i = 0; i < LAST_SEEN_BUFFER_SIZE; i++)
            if(last_seen[i] == combined_id)
                seen = true;

        // if seen, queue a new buffer, and mark it as seen
        if(!seen
#ifdef FILTER
        && (MicroBitPeridoRadio::instance->appId == p->app_id || MicroBitPeridoRadio::instance->appId == 0)
#endif
        )
        {
            MicroBitPeridoRadio::instance->copyRxBuf();
#ifdef TRACE
            process_packet(MicroBitPeridoRadio::instance->rxBuf);
#endif

            last_seen[last_seen_index] = combined_id;
            last_seen_index = (last_seen_index + 1) %  LAST_SEEN_BUFFER_SIZE;
        }
#ifdef TRACE
        set_store_disable_gpio(0);
#endif
#endif // end DISABLE_STORE
    }

    if(radio_status & RADIO_STATUS_DISABLE)
    {
        PERIDO_LOG_FLAGS(radio_status);
        PERIDO_LOG_FLAGS(NRF_RADIO->STATE);
#ifdef TRACE
        set_store_disable_gpio(1);
#endif
        NRF_RADIO->EVENTS_END = 0;
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->EVENTS_ADDRESS = 0;

        // Turn off the transceiver.
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE = 1;

        radio_status = (radio_status & (HIGH_LEVEL_STATE_MASK | RADIO_STATUS_RX_EN | RADIO_STATUS_TX_EN));
        PERIDO_LOG_FLAGS(radio_status);

        // instead of relying on timer accuracy, we busy wait instead to ensure retransmissions are as close as possible.
        if (radio_status & RADIO_STATUS_FORWARD && radio_status & RADIO_STATUS_TX_EN)
        {
            // disable takes 10 us , account for variabilities
            accurate_delay_us(20);
            HW_ASSERT(0, 11);
            PERIDO_ASSERT(NRF_RADIO->EVENTS_DISABLED == 1);
            NRF_RADIO->EVENTS_DISABLED = 0;
            PERIDO_ASSERT(!(radio_status & (RADIO_STATUS_RX_EN|RADIO_STATUS_RX_RDY|RADIO_STATUS_RECEIVING|RADIO_STATUS_RECEIVE)));
            // WE DON'T WANT THE ADDRESS EVENT
            NRF_RADIO->INTENCLR = 0x0000000A;
            NRF_RADIO->INTENSET = 0x00000008;


            NRF_RADIO->EVENTS_READY = 0;
            NRF_RADIO->TASKS_TXEN = 1;

            // tx enable takes 166 us (account for variabilities)
            accurate_delay_us(TX_ENABLE_TIME + 200);

            PERIDO_UNSET_FLAGS(RADIO_STATUS_TX_EN | RADIO_STATUS_DISABLED);
            PERIDO_SET_FLAGS(RADIO_STATUS_TX_RDY);
            HW_ASSERT(10, 55);

            goto state_machine_start;
        }

        PERIDO_SET_FLAGS(RADIO_STATUS_DISABLED);
        MicroBitPeridoRadio::instance->timer.setCompare(STATE_MACHINE_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(STATE_MACHINE_CHANNEL) + RX_TX_DISABLE_TIME);
#ifdef TRACE
        set_store_disable_gpio(0);
#endif
        return;
    }
}
int interrupt_count = 0;
extern "C" void RADIO_IRQHandler(void)
{
    PERIDO_LOG_FLAGS(radio_status);
    interrupt_count++;
    radio_state_machine();
}

// used to initiate transmission if the window is clear.
void tx_callback()
{
#ifdef TRACE
    set_tx_callback_gpio(1);
#endif
    if (radio_status & (RADIO_STATUS_SLEEPING | RADIO_STATUS_FORWARD | RADIO_STATUS_RECEIVING))
    {
#ifdef TRACE
        set_tx_callback_gpio(0);
#endif
        return;
    }

    if (!(radio_status & RADIO_STATUS_RX_RDY))
    {
#ifdef TRACE
        set_tx_callback_gpio(0);
#endif
        return;
    }

    NVIC_DisableIRQ(RADIO_IRQn);
    // no one else has transmitted recently, and we are not receiving, we can transmit
    if(MicroBitPeridoRadio::instance->getCurrentTxBuf() != NULL)
    {
        radio_status = (radio_status & (RADIO_STATUS_DISCOVERING | RADIO_STATUS_FIRST_PACKET)) | RADIO_STATUS_DISABLE | RADIO_STATUS_TX_EN | RADIO_STATUS_TRANSMIT;
        PERIDO_LOG_FLAGS(radio_status);
        radio_state_machine();
#ifdef TRACE
        set_tx_callback_gpio(0);
#endif
        NVIC_EnableIRQ(RADIO_IRQn);
        return;
    }

    if (radio_status & RADIO_STATUS_DISCOVERING)
    {
        MicroBitPeridoRadio::instance->timer.setCompare(CHECK_TX_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL) + DISCOVERY_TX_BACKOFF_TIME + microbit_random(DISCOVERY_TX_BACKOFF_TIME));
        NVIC_EnableIRQ(RADIO_IRQn);
#ifdef TRACE
        set_tx_callback_gpio(0);
#endif
        return;
    }
    NVIC_EnableIRQ(RADIO_IRQn);
#ifdef TRACE
    set_tx_callback_gpio(0);
#endif

    // uint32_t max_sleep = ((periods[network_period_idx] / 4) * 1000);
    // uint32_t tx_backoff = PERIDO_WAKE_THRESHOLD_MID;
    // tx_backoff +=  microbit_random(max_sleep);
    // MicroBitPeridoRadio::instance->timer.setCompare(CHECK_TX_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL) + tx_backoff);

}

// used to begin a transmission window
void go_to_sleep()
{
// #ifdef TRACE
// #ifndef TRACE_WAKE
//     set_wake_gpio(1);
// #endif
// #endif
// #ifdef TRACE
// #ifndef TRACE_WAKE
//     set_wake_gpio(1);
// #endif
// #endif
    if (radio_status & (RADIO_STATUS_RECEIVING | RADIO_STATUS_TRANSMIT | RADIO_STATUS_FORWARD))
    {
// #ifdef TRACE
// #ifndef TRACE_WAKE
//         set_wake_gpio(0);
// #endif
// #endif
// #ifdef TRACE
// #ifndef TRACE_WAKE
//     set_wake_gpio(0);
// #endif
// #endif
#ifdef TRACE_WAKE
        set_wake_gpio(0);
        set_wake_gpio(1);
#endif
        return;
    }

    NVIC_DisableIRQ(RADIO_IRQn);

    if (radio_status & RADIO_STATUS_EXPECT_RESPONSE)
    {
        no_response_count++;
        PERIDO_UNSET_FLAGS(RADIO_STATUS_EXPECT_RESPONSE);
    }

    PERIDO_SET_FLAGS(RADIO_STATUS_SLEEPING | RADIO_STATUS_DISABLE);

#ifdef TRACE
#ifdef TRACE_WAKE
    set_wake_gpio(0);
#endif
#endif

// #ifdef TRACE
// #ifndef TRACE_WAKE
//         set_wake_gpio(0);
// #endif
// #endif
    radio_state_machine();
    NVIC_EnableIRQ(RADIO_IRQn);

    if (keep_alive_count >= keep_alive_match)
    {
        keep_alive_count = 0;
        PERIDO_SET_FLAGS(RADIO_STATUS_QUEUE_KEEP_ALIVE);
    }

// #ifdef TRACE
// #ifndef TRACE_WAKE
//     set_wake_gpio(0);
// #endif
// #endif

// #ifdef TRACE
// #ifndef TRACE_WAKE
//     set_wake_gpio(0);
// #endif
// #endif
}

// used to begin a transmission window
void wake_up()
{
    period_start_cc = MicroBitPeridoRadio::instance->timer.captureCounter(WAKE_UP_CHANNEL);
    current_cc = period_start_cc + ((periods[network_period_idx] * 1000));

    if (no_response_count > NO_RESPONSE_THRESHOLD)
    {
        PERIDO_SET_FLAGS(RADIO_STATUS_DISCOVERING);
        no_response_count = 0;
    }

    // we're still exchanging packets - come back in another period amount.
    if (!(radio_status & RADIO_STATUS_SLEEPING))
    {
#ifdef TRACE
#ifdef TRACE_WAKE
        set_wake_gpio(0);
#endif
#endif
         // 3 / 4 of the period.
        uint32_t max_sleep = ((periods[network_period_idx] / 4) * 3000);
        uint32_t tx_backoff = PERIDO_WAKE_THRESHOLD_MID;
        tx_backoff +=  microbit_random(max_sleep);
        // don't forget to check tx if we've bled into the next period.
        // PERIDO_SET_FLAGS( RADIO_STATUS_FIRST_PACKET);
        MicroBitPeridoRadio::instance->timer.setCompare(CHECK_TX_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL) + tx_backoff);
        MicroBitPeridoRadio::instance->timer.setCompare(WAKE_UP_CHANNEL, current_cc);
#ifdef TRACE
#ifdef TRACE_WAKE
        set_wake_gpio(1);
#endif
#endif
        return;
    }

#ifdef TRACE
#ifdef TRACE_WAKE
        set_wake_gpio(1);
#endif
#endif

    PERIDO_UNSET_FLAGS(RADIO_STATUS_SLEEPING | RADIO_STATUS_WAKE_CONFIGURED);
    PERIDO_SET_FLAGS( RADIO_STATUS_RX_EN | RADIO_STATUS_FIRST_PACKET);

    if (radio_status & RADIO_STATUS_DISCOVERING)
    {
        MicroBitPeridoRadio::instance->timer.setCompare(CHECK_TX_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL) + microbit_random(1000));
        MicroBitPeridoRadio::instance->timer.setCompare(WAKE_UP_CHANNEL, current_cc);
        discovery_tx_time = DISCOVERY_TX_BACKOFF_TIME_RUNNING;
    }
    else
    {
        // 3 / 4 of the period.
        uint32_t max_sleep = ((periods[network_period_idx] / 4) * 3000);
        uint32_t tx_backoff = PERIDO_WAKE_THRESHOLD_MID;

        tx_backoff +=  microbit_random(max_sleep);

        MicroBitPeridoRadio::instance->timer.setCompare(CHECK_TX_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(CHECK_TX_CHANNEL) + tx_backoff);
#ifndef DISABLE_SLEEP
        MicroBitPeridoRadio::instance->timer.setCompare(GO_TO_SLEEP_CHANNEL, MicroBitPeridoRadio::instance->timer.captureCounter(GO_TO_SLEEP_CHANNEL) + max_sleep);
#endif
        MicroBitPeridoRadio::instance->timer.setCompare(WAKE_UP_CHANNEL, current_cc);
    }

    radio_state_machine();
}

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
void timer_callback(uint8_t state)
{
    PERIDO_LOG_FLAGS(state);
#ifdef TRACE
#ifndef TRACE_WAKE
        set_wake_gpio(1);
#endif
#endif
    if(state & (1 << STATE_MACHINE_CHANNEL))
        radio_state_machine();

    if(state & (1 << WAKE_UP_CHANNEL))
        wake_up();

    if(state & (1 << CHECK_TX_CHANNEL))
        tx_callback();

    if(state & (1 << GO_TO_SLEEP_CHANNEL))
        go_to_sleep();
#ifdef TRACE
#ifndef TRACE_WAKE
    set_wake_gpio(0);
#endif
#endif
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
    PeridoFrameBuffer buf;

    buf.id = microbit_random(65535);
    buf.length = 0 + MICROBIT_PERIDO_HEADER_SIZE - 1; // keep alive has no content.
    buf.app_id = appId;
    buf.namespace_id = 0;
    buf.flags |= MICROBIT_PERIDO_FRAME_KEEP_ALIVE_FLAG;
    buf.ttl = 2;
    buf.initial_ttl = 2;
    buf.time_since_wake = 0;
    buf.period = 0;

    return queueTxBuf(&buf);
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

    NRF_RADIO->PCNF1 = 0x00040000 | MICROBIT_PERIDO_MAX_PACKET_SIZE;

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
    // NRF_RADIO->DATAWHITEIV = 0x18;

    // Set up the RADIO module to read and write from our internal buffer.
    NRF_RADIO->PACKETPTR = (uint32_t)rxBuf;

    // Configure the hardware to issue an interrupt whenever a task is complete (e.g. send/receive)
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_SetPriority(RADIO_IRQn, 0);
    NVIC_EnableIRQ(RADIO_IRQn);

    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->EVENTS_END = 0;

    PERIDO_SET_FLAGS(RADIO_STATUS_DISABLED | RADIO_STATUS_DISCOVERING | RADIO_STATUS_SLEEPING);
    timer.setCompare(WAKE_UP_CHANNEL, timer.captureCounter(WAKE_UP_CHANNEL) + periods[periodIndex] * 1000);

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
    if (radio_status & RADIO_STATUS_QUEUE_KEEP_ALIVE)
    {
        queueKeepAlive();
        PERIDO_UNSET_FLAGS(RADIO_STATUS_QUEUE_KEEP_ALIVE);
    }

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

        LOG_STRING("PACKET REC. NAMESPACE:");
        LOG_NUM(p->namespace_id);
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
    LOG_STRING("SEND BUFFFF ");
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

    LOG_STRING("OUT");

    return send(&buf);
}

uint16_t MicroBitPeridoRadio::generateId(uint8_t app_id, uint8_t namespace_id)
{
    LOG_STRING("GEN_ID: ");
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

    LOG_NUM(new_id);

    return new_id;
}

#endif
