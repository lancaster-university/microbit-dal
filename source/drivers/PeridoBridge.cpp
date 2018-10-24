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

const MicroBitImage line = MicroBitImage("0, 0, 0\n255, 255, 255\n0, 0, 0\n");

const MicroBitImage tick = MicroBitImage("0, 0, 0, 255\n255, 0, 255, 0\n0, 255, 0, 0\n");

const MicroBitImage cross = MicroBitImage("255, 0, 255\n 0, 255, 0\n 255, 0, 255\n");

void PeridoBridge::sendHelloPacket()
{
    DynamicType t;

    t.appendInteger(0); // status ok! change in future if things aren't ok...
    t.appendString(SCHOOL_ID);
    t.appendString(HUB_ID);

    serialPacket.request_id = 0;
    serialPacket.app_id = 0;
    serialPacket.namespace_id = 0;
    serialPacket.payload[0] = REQUEST_TYPE_HELLO;

    memcpy(&serialPacket.payload[1], t.getBytes(), t.length());
    sendSerialPacket(t.length() + BRIDGE_SERIAL_PACKET_HEADER_SIZE + 2); // +2 for the id (which is normally included in perido frame buffer)
}

void PeridoBridge::handleHelloPacket(DataPacket* pkt, uint16_t len)
{
    DynamicType dt = DynamicType(len - CLOUD_HEADER_SIZE, pkt->payload, 0);

    int status = dt.getInteger(0);

    // clear the pixels using the biggest image
    for (int j = 0; j < tick.getHeight(); j++)
        for (int i = 0; i < tick.getWidth(); i++)
            display.image.setPixelValue(i, j, 0);

    if (status == MICROBIT_OK)
        display.print(tick);
    else
        display.print(cross);
}

void PeridoBridge::updatePacketCount()
{
    uint8_t countImageData[PACKET_COUNT_BIT_RESOLUTION] = { 0 };

    int arrayCount = 0;
    for (int i = PACKET_COUNT_BIT_RESOLUTION-1; i > -1; i--)
    {
        countImageData[i] = (packetCount & (1 << arrayCount)) ? 255 : 0;
        arrayCount++;
    }

    // reset packet count if it's off the end of the display
    if (this->packetCount > PACKET_COUNT_DISPLAY_RESOLUTION)
        this->packetCount = 0;

    MicroBitImage count(5, 1, countImageData);

    // clear pixels
    for (int i = 0; i < MICROBIT_DISPLAY_WIDTH; i++)
        display.image.setPixelValue(i, PACKET_COUNT_PASTE_ROW, 0);

    // paste our new count
    display.image.paste(count, PACKET_COUNT_PASTE_COLUMN, PACKET_COUNT_PASTE_ROW);
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

        this->packetCount++;
    }

    updatePacketCount();
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

    DataPacket* temp = (DataPacket*)&serialPacket.request_id;

    // calculate the pure packet length.
    len -= BRIDGE_SERIAL_PACKET_HEADER_SIZE;

    if (temp->request_type & REQUEST_TYPE_HELLO)
    {
        handleHelloPacket(temp, len);
    }
    else
    {
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
    }

    change_display();
}

void PeridoBridge::enable()
{
    display.print(line);

    // max size of a DataPacket
    serial.setRxBufferSize(254);

    // configure an event for SLIP_END
    serial.eventOn((char)SLIP_END);

    sendHelloPacket();

    // listen to everything.
    radio.cloud.setBridgeMode(true);
}

PeridoBridge::PeridoBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b, MicroBitDisplay& display) : radio(r), serial(s), display(display)
{
    this->packetCount = 0;
    b.listen(MICROBIT_RADIO_ID_CLOUD, MICROBIT_EVT_ANY, this, &PeridoBridge::onRadioPacket, MESSAGE_BUS_LISTENER_IMMEDIATE);
    b.listen(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_DELIM_MATCH, this, &PeridoBridge::onSerialPacket);
}

#endif