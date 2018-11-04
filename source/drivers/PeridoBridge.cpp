#include "MicroBitConfig.h"

#if MICROBIT_RADIO_VERSION == MICROBIT_RADIO_PERIDO

// extern void log_string_ch(const char*);
// extern void log_num(int);

// #define LOG_STRING(str) (log_string_ch(str))
// #define LOG_NUM(str) (log_num(str))

#define LOG_STRING(...) ((void)0)
#define LOG_NUM(...) ((void)0)

#include "PeridoBridge.h"
#include "EventModel.h"

#define SLIP_END                0xC0
#define SLIP_ESC                0xDB
#define SLIP_ESC_END            0xDC
#define SLIP_ESC_ESC            0xDD

const char* HUB_ID = "M1cR0B1THuBs";
const char* SCHOOL_ID = "M1cR0B1TSCHO";

const MicroBitImage smile_small = MicroBitImage("0, 0, 0, 0, 0\n0, 255, 0, 255, 0\n0, 0, 0, 0, 0\n255, 0, 0, 0, 255\n0, 255, 255, 255, 0\n");
const MicroBitImage smile_big = MicroBitImage("0, 0, 0, 0, 0\n0, 255, 0, 255, 0\n0, 0, 0, 0, 0\n255, 255, 255, 255, 255\n0, 255, 255, 255, 0\n");
const MicroBitImage neutral_small = MicroBitImage("0, 0, 0, 0, 0\n0, 255, 0, 255, 0\n0, 0, 0, 0, 0\n0, 255, 255, 255, 0\n0, 255, 255, 255, 0\n");
const MicroBitImage neutral_big = MicroBitImage("0, 0, 0, 0, 0\n0, 255, 0, 255, 0\n0, 0, 0, 0, 0\n255, 255, 255, 255, 255\n0, 0, 0, 0, 0\n");
const MicroBitImage sad_small = MicroBitImage("0, 0, 0, 0, 0\n0, 255, 0, 255, 0\n0, 0, 0, 0, 0\n0, 255, 255, 255, 0\n255, 0, 0, 0, 255\n");
const MicroBitImage sad_big = MicroBitImage("0, 0, 0, 0, 0\n0, 255, 0, 255, 0\n0, 0, 0, 0, 0\n0, 255, 255, 255, 0\n255, 255, 255, 255, 255\n");

bool test_mode = true;

void PeridoBridge::queueTestResponse()
{
    LOG_STRING("Q TEST RESP");
    CloudDataItem* cloudData = new CloudDataItem;
    PeridoFrameBuffer* buf = new PeridoFrameBuffer;

    DataPacket* dp = (DataPacket*)&serialPacket.request_id;

    DynamicType response;
    response.appendString("99");


    LOG_STRING("APPEND DUMMY");

    int len = CLOUD_HEADER_SIZE + response.length();
    memcpy(dp->payload, response.getBytes(), response.length());

    buf->id = radio.generateId(serialPacket.app_id, serialPacket.namespace_id);
    buf->length = len + (MICROBIT_PERIDO_HEADER_SIZE - 1);
    buf->app_id = serialPacket.app_id;
    buf->namespace_id = serialPacket.namespace_id;
    buf->ttl = 4;
    buf->initial_ttl = 4;
    buf->time_since_wake = 0;
    buf->period = 0;

    LOG_STRING("COPY!");
    memcpy(buf->payload, &serialPacket.request_id, len);

    cloudData->packet = buf;
    // we queue and flag the packet as waiting to send, it expects and ack, but no response
    cloudData->status = DATA_PACKET_WAITING_FOR_SEND | DATA_PACKET_EXPECT_NO_RESPONSE;
    cloudData->no_response_count = 0;
    cloudData->retry_count = 0;

    LOG_STRING("ADD TX QUEUE:");
    LOG_NUM(buf->app_id);
    LOG_NUM(buf->namespace_id);
    LOG_NUM(((DataPacket*)buf->payload)->request_id);
    // cloud data will be deleted automatically.
    radio.cloud.addToTxQueue(cloudData);
}

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

    display.clear();

    if (status == MICROBIT_OK)
    {
        this->status = BRIDGE_STATUS_OK;
        display.print(smile_small);
    }
    else
    {
        this->status = BRIDGE_STATUS_ERROR;
        display.print(sad_small);
    }

    status |= BRIDGE_DISPLAY_BIG;
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

        serialPacket.app_id = packet->app_id;
        serialPacket.namespace_id = packet->namespace_id;
        // first two bytes of the payload nicely contain the id.
        memcpy(&serialPacket.request_id, packet->payload, packet->length - MICROBIT_PERIDO_HEADER_SIZE);

        LOG_STRING("RX PACKET: ");
        LOG_NUM(serialPacket.app_id);
        LOG_NUM(serialPacket.namespace_id);
        LOG_NUM(serialPacket.request_id);

        if (test_mode)
            queueTestResponse();
        else
            // forward the packet over serial
            sendSerialPacket((packet->length - (MICROBIT_PERIDO_HEADER_SIZE - 1)) + BRIDGE_SERIAL_PACKET_HEADER_SIZE);

        // we are now finished with the packet..
        delete r;

        this->packetCount++;
    }

    display.clear();

    // big smile
    if (this->status & BRIDGE_STATUS_OK)
    {
        if (this->status & BRIDGE_DISPLAY_BIG)
        {
            display.image.paste(smile_small);
            this->status &= ~BRIDGE_DISPLAY_BIG;
        }
        else
        {
            display.image.paste(smile_big);
            this->status |= BRIDGE_DISPLAY_BIG;
        }
    }
    else if (this->status & BRIDGE_STATUS_ERROR)
    {
        if (this->status & BRIDGE_DISPLAY_BIG)
        {
            display.image.paste(sad_small);
            this->status &= ~BRIDGE_DISPLAY_BIG;
        }
        else
        {
            display.image.paste(sad_big);
            this->status |= BRIDGE_DISPLAY_BIG;
        }
    }
    else
    {
        if (this->status & BRIDGE_DISPLAY_BIG)
        {
            display.image.paste(neutral_small);
            this->status &= ~BRIDGE_DISPLAY_BIG;
        }
        else
        {
            display.image.paste(neutral_big);
            this->status |= BRIDGE_DISPLAY_BIG;
        }
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
        // we queue and flag the packet as waiting to send, it expects and ack, but no response
        cloudData->status = DATA_PACKET_WAITING_FOR_SEND | DATA_PACKET_EXPECT_NO_RESPONSE;

        cloudData->no_response_count = 0;
        cloudData->retry_count = 0;

        // cloud data will be deleted automatically.
        radio.cloud.addToTxQueue(cloudData);
    }
}

void PeridoBridge::enable()
{
    display.print(neutral_big);

    // max size of a DataPacket
    serial.setRxBufferSize(512);

    // configure an event for SLIP_END
    serial.eventOn((char)SLIP_END);

    sendHelloPacket();

    // listen to everything.
    radio.cloud.setBridgeMode(true);
}

PeridoBridge::PeridoBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b, MicroBitDisplay& display) : radio(r), serial(s), display(display)
{
    this->packetCount = 0;
    this->status = 0;
    b.listen(MICROBIT_RADIO_ID_CLOUD, MICROBIT_EVT_ANY, this, &PeridoBridge::onRadioPacket, MESSAGE_BUS_LISTENER_IMMEDIATE);
    b.listen(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_DELIM_MATCH, this, &PeridoBridge::onSerialPacket);
}

#endif