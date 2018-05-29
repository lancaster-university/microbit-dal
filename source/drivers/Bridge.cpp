#include "MicroBitConfig.h"

#if MICROBIT_RADIO_VERSION == MICROBIT_RADIO_MODIFIED

#include "Bridge.h"
#include "EventModel.h"

#define SLIP_END                0xC0
#define SLIP_ESC                0xDB
#define SLIP_ESC_END            0xDC
#define SLIP_ESC_ESC            0xDD

#define HISTORY_COUNT           20
#define APP_ID_MSK              0xFFFF0000
#define PACKET_ID_MSK           0x0000FFFF

extern void log_string_priv(const char *);
extern void log_num_priv(int num);
static uint32_t id_history[HISTORY_COUNT] = { 0 };
static uint16_t historyIndexHead = 0;

bool Bridge::searchHistory(uint16_t app_id, uint16_t id)
{
    for (int idx = 0; idx < HISTORY_COUNT; idx++)
    {
        if (((id_history[idx] & APP_ID_MSK) >> 16) == app_id && (id_history[idx] & PACKET_ID_MSK) == id)
            return true;
    }

    return false;
}

void Bridge::addToHistory(uint16_t app_id, uint16_t id)
{
    id_history[historyIndexHead] = ((app_id << 16) | id);
    historyIndexHead = (historyIndexHead + 1) % HISTORY_COUNT;
}

void Bridge::test_send(DataPacket* p)
{
    DataPacket* packet = (DataPacket *)malloc(sizeof(DataPacket));

    memcpy(packet, p, sizeof(DataPacket));

    packet->request_type = REQUEST_STATUS_OK;
    packet->len = 0;

    packet->status = DATA_PACKET_WAITING_FOR_SEND;
    int ret = radio.cloud.addToTxQueue(packet);

}

void Bridge::onRadioPacket(MicroBitEvent e)
{
    DataPacket* r = radio.cloud.recvRaw(e.value);
    // log_string_priv("Radio_PACKET");

    if (r == NULL)
        return;

    // send an ACK immediately
    DataPacket* ack = new DataPacket();
    ack->app_id = r->app_id;
    ack->id = r->id;
    ack->request_type = REQUEST_STATUS_ACK;
    ack->status = 0;
    ack->len = 0;

    radio.cloud.sendDataPacket(ack);
    delete ack;

    uint8_t* packetPtr = (uint8_t *)r;
    uint16_t len = r->len;

    bool seen = searchHistory(r->app_id, r->id);

    if (!seen)
    {
        // log_string_priv("not_seen");
        addToHistory(r->app_id, r->id);

        for (uint16_t i = 0; i < len; i++)
        {
            if (packetPtr[i] == SLIP_ESC)
            {
                serial.putc(SLIP_ESC);
                serial.putc(SLIP_ESC_ESC);
                continue;
            }

            if(packetPtr[i] == SLIP_END)
            {
                serial.putc(SLIP_ESC);
                serial.putc(SLIP_ESC_END);
                continue;
            }

            serial.putc(packetPtr[i]);
        }

        serial.putc(SLIP_END);
    }

    delete r;
}

void Bridge::onSerialPacket(MicroBitEvent)
{
    log_string_priv("REC_SER");
    DataPacket* packet = (DataPacket*) malloc(sizeof(DataPacket));
    uint8_t* packetPtr = (uint8_t*)packet;

    char c = 0;

    while ((c = serial.read()) != SLIP_END)
    {
        if (packet->len >= MICROBIT_RADIO_MAX_PACKET_SIZE)
            continue;

        if (c == SLIP_ESC)
        {
            char next = serial.read();

            if (next == SLIP_ESC_END)
                c = SLIP_END;
            else if (next == SLIP_ESC_ESC)
                c = SLIP_ESC;
            else
            {
                *packetPtr++ = c;
                *packetPtr++ = next;
                packet->len += 2;
            }

            continue;
        }

        packet->len++;

        *packetPtr++ = c;
    }

    packet->len -= CLOUD_HEADER_SIZE;

    log_string_priv("TXING:");
    log_num_priv(packet->id);
    log_num_priv(packet->app_id);

    packet->status = DATA_PACKET_WAITING_FOR_SEND | DATA_PACKET_EXPECT_NO_RESPONSE;
    int ret = radio.cloud.addToTxQueue(packet);

    serial.eventOn((char)SLIP_END);
}

Bridge::Bridge(Radio& r, MicroBitSerial& s, MicroBitMessageBus& b) : radio(r), serial(s)
{
    s.putc(SLIP_END);

    memset(id_history, 0, sizeof(uint32_t) * HISTORY_COUNT);

    log_string_priv("BRIDGE_INIT");

    // listen to everything.
    r.cloud.setBridgeMode(true);

    b.listen(MICROBIT_RADIO_ID_CLOUD, MICROBIT_EVT_ANY, this, &Bridge::onRadioPacket, MESSAGE_BUS_LISTENER_IMMEDIATE);
    b.listen(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_DELIM_MATCH, this, &Bridge::onSerialPacket);

    // max size of a DataPacket
    s.setRxBufferSize(254);

    // configure an event for SLIP_END
    s.eventOn((char)SLIP_END);
}

#endif