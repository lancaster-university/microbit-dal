#include "MicroBitConfig.h"

#if MICROBIT_RADIO_VERSION == MICROBIT_RADIO_PERIDO

#include "PeridoBridge.h"
#include "EventModel.h"

#define SLIP_END                0xC0
#define SLIP_ESC                0xDB
#define SLIP_ESC_END            0xDC
#define SLIP_ESC_ESC            0xDD

const char* HUB_ID = "M1cR0B1THuBs";
const char* SCHOOL_ID = "M1cR0B1TSCHO";

void PeridoBridge::sendHelloPacket()
{
    DynamicType t;

    t.appendString(SCHOOL_ID);
    t.appendString(HUB_ID);

    serialPacket.request_id = 0;
    serialPacket.app_id = 0;
    serialPacket.namespace_id = 0;
    serialPacket.payload[0] = REQUEST_TYPE_HELLO;

    memcpy(&serialPacket.payload[1], t.getBytes(), t.length());
    sendSerialPacket(t.length() + BRIDGE_SERIAL_PACKET_HEADER_SIZE + 2); // +2 for the id (which is normally included in perido frame buffer)
}

void PeridoBridge::sendSerialPacket(uint16_t len)
{
    uint8_t* packetPtr = (uint8_t *)&serialPacket;

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

void PeridoBridge::onRadioPacket(MicroBitEvent)
{
    CloudDataItem* r = NULL;

    while((r = radio.cloud.recvRaw()))
    {
        PeridoFrameBuffer* packet = r->packet;
        DataPacket* data = (DataPacket*)packet->payload;

        radio.cloud.sendAck(data->request_id, packet->app_id, packet->namespace_id);

        serialPacket.app_id = packet->app_id;
        serialPacket.namespace_id = packet->namespace_id;
        // first two bytes of the payload nicely contain the id.
        memcpy(&serialPacket.request_id, packet->payload, packet->length - MICROBIT_PERIDO_HEADER_SIZE);

        // forward the packet over serial
        sendSerialPacket((packet->length - (MICROBIT_PERIDO_HEADER_SIZE - 1)) + BRIDGE_SERIAL_PACKET_HEADER_SIZE);

        // we are now finished with the packet..
        delete r;
    }
}

extern void change_display();
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

    buf->id = radio.generateId(serialPacket.app_id, serialPacket.namespace_id);
    buf->length = len + (MICROBIT_PERIDO_HEADER_SIZE - 1);
    buf->app_id = serialPacket.app_id;
    buf->namespace_id = serialPacket.namespace_id;
    buf->ttl = 4;
    buf->initial_ttl = 4;
    buf->time_since_wake = 0;
    buf->period = 0;
    memcpy(buf->payload, &serialPacket.request_id, len);

    cloudData->packet = buf;
    cloudData->status = DATA_PACKET_WAITING_FOR_SEND | DATA_PACKET_EXPECT_NO_RESPONSE;

    // cloud data will be deleted automatically.
    radio.cloud.addToTxQueue(cloudData);
    change_display();
}

PeridoBridge::PeridoBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b) : radio(r), serial(s)
{
    sendHelloPacket();

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