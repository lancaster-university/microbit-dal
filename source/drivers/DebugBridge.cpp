#include "MicroBitConfig.h"

#if MICROBIT_RADIO_VERSION == MICROBIT_RADIO_PERIDO

// extern void log_string_ch(const char*);
// extern void log_num(int);

// #define LOG_STRING(str) (log_string_ch(str))
// #define LOG_NUM(str) (log_num(str))

#define LOG_STRING(...) ((void)0)
#define LOG_NUM(...) ((void)0)

#include "DebugBridge.h"
#include "EventModel.h"
#include "MicroBitSystemTimer.h"

void DebugBridge::onRadioPacket(MicroBitEvent)
{
    CloudDataItem* r = NULL;

    while((r = radio.cloud.recvRaw()))
    {
        PeridoFrameBuffer* packet = r->packet;
        DataPacket* dp = (DataPacket*)packet->payload;
        if (dp->request_type & REQUEST_STATUS_ERROR || dp->request_type & REQUEST_STATUS_OK)
            serial.printf("RESP: aid: %d nsid: %d rid: %d time: %d\r\n", packet->app_id, packet->namespace_id, dp->request_id, (int)system_timer_current_time());
        else
            serial.printf("REQ: aid: %d nsid: %d rid: %d time: %d\r\n", packet->app_id, packet->namespace_id, dp->request_id, (int)system_timer_current_time());
        delete r;
    }
}

void DebugBridge::enable()
{
    // listen to everything.
    radio.cloud.setBridgeMode(true);
}

DebugBridge::DebugBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b, MicroBitDisplay& display) : radio(r), serial(s), display(display)
{
    this->packetCount = 0;
    this->status = 0;
    b.listen(MICROBIT_RADIO_ID_CLOUD, MICROBIT_EVT_ANY, this, &DebugBridge::onRadioPacket, MESSAGE_BUS_LISTENER_IMMEDIATE);
}

#endif