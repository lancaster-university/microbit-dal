#include "MicroBitConfig.h"

#if MICROBIT_RADIO_VERSION == MICROBIT_RADIO_PERIDO

#include "PeridoBridge.h"
#include "EventModel.h"

#define SLIP_END                0xC0
#define SLIP_ESC                0xDB
#define SLIP_ESC_END            0xDC
#define SLIP_ESC_ESC            0xDD

#define HISTORY_COUNT           20
#define APP_ID_MSK              0xFFFF0000
#define PACKET_ID_MSK           0x0000FFFF

static uint32_t id_history[HISTORY_COUNT] = { 0 };
static uint16_t historyIndexHead = 0;

bool PeridoBridge::searchHistory(uint16_t app_id, uint16_t id)
{
    for (int idx = 0; idx < HISTORY_COUNT; idx++)
    {
        if (((id_history[idx] & APP_ID_MSK) >> 16) == app_id && (id_history[idx] & PACKET_ID_MSK) == id)
            return true;
    }

    return false;
}

void PeridoBridge::addToHistory(uint16_t app_id, uint16_t id)
{
    id_history[historyIndexHead] = ((app_id << 16) | id);
    historyIndexHead = (historyIndexHead + 1) % HISTORY_COUNT;
}

void PeridoBridge::test_send(DataPacket*)
{

}

void PeridoBridge::onRadioPacket(MicroBitEvent e)
{
    CloudDataItem* r = NULL;

    while((r = radio.cloud.recvRaw()))
    {
        PeridoFrameBuffer* packet = r->packet;
        radio.cloud.sendAck(packet->id, packet->app_id, packet->namespace_id);

        serialPacket.app_id = packet->app_id;
        serialPacket.namespace_id = packet->namespace_id;
        serialPacket.id = packet->id;

        memcpy(serialPacket.payload, packet->payload, packet->length - MICROBIT_PERIDO_HEADER_SIZE);

        uint8_t* packetPtr = (uint8_t *)&serialPacket;
        uint16_t len = packet->length - MICROBIT_PERIDO_HEADER_SIZE + BRIDGE_SERIAL_PACKET_HEADER_SIZE;

        bool seen = searchHistory(packet->app_id, packet->id);

        if (!seen)
        {
            addToHistory(packet->app_id, packet->id);

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

        // we are now finished with the packet..
        delete r;
    }
}

void PeridoBridge::onSerialPacket(MicroBitEvent)
{
    uint8_t* packetPtr = (uint8_t*)&serialPacket;
    uint16_t len = 0;
    char c = 0;

    while ((c = serial.read()) != SLIP_END)
    {
        if (len >= MICROBIT_PERIDO_MAX_PACKET_SIZE)
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
                len += 2;
            }

            continue;
        }

        len++;

        *packetPtr++ = c;
    }

    // calculate the pure packet length.
    len -= BRIDGE_SERIAL_PACKET_HEADER_SIZE;

    CloudDataItem* cloudData = new CloudDataItem;
    PeridoFrameBuffer* buf = new PeridoFrameBuffer;

    buf->id = serialPacket.id;
    buf->length = len + MICROBIT_PERIDO_HEADER_SIZE - 1;
    buf->app_id = serialPacket.app_id;
    buf->namespace_id = serialPacket.namespace_id;
    buf->ttl = 4;
    buf->initial_ttl = 4;
    buf->time_since_wake = 0;
    buf->period = 0;
    memcpy(buf->payload, serialPacket.payload, len);

    cloudData->packet = buf;
    cloudData->status = DATA_PACKET_WAITING_FOR_SEND;

    int ret = radio.cloud.addToTxQueue(cloudData);
    delete cloudData; // includes deletion of buf.
}

PeridoBridge::PeridoBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b) : radio(r), serial(s)
{
    s.putc(SLIP_END);

    memset(id_history, 0, sizeof(uint32_t) * HISTORY_COUNT);

    // listen to everything.
    r.cloud.setBridgeMode(true);

    b.listen(MICROBIT_RADIO_ID_CLOUD, MICROBIT_EVT_ANY, this, &PeridoBridge::onRadioPacket, MESSAGE_BUS_LISTENER_IMMEDIATE);
    b.listen(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_DELIM_MATCH, this, &PeridoBridge::onSerialPacket);

    // max size of a DataPacket
    s.setRxBufferSize(254);

    // configure an event for SLIP_END
    s.eventOn((char)SLIP_END);
}

#endif