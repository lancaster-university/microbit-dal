#ifndef PERIDO_BRIDGE_H
#define PERIDO_BRIDGE_H

#include "MicroBitPeridoRadio.h"
#include "MicroBitSerial.h"
#include "MicroBitMessageBus.h"
#include "MicroBitDisplay.h"
#include "MicroBitComponent.h"

// the header is only the first two bytes, as the id is placed inside the payload
#define BRIDGE_SERIAL_PACKET_HEADER_SIZE        2

#define BRIDGE_STATUS_OK                        0x01
#define BRIDGE_STATUS_ERROR                     0x02
#define BRIDGE_DISPLAY_BIG                      0x10


#define BLINK_POSITION_X                        3
#define BLINK_POSITION_Y                        1

struct PeridoBridgeSerialPacket
{
    uint8_t app_id;
    uint8_t namespace_id;
    uint16_t request_id;

    uint8_t payload[MICROBIT_PERIDO_MAX_PACKET_SIZE];
}__attribute((packed));

class PeridoBridge : public MicroBitComponent
{
    MicroBitPeridoRadio& radio;
    MicroBitSerial& serial;
    MicroBitDisplay& display;

    PeridoBridgeSerialPacket serialPacket; // holds data to be transmitted or received

    uint32_t packetCount;

    void sendHelloPacket();
    void handleHelloPacket(DataPacket* pkt, uint16_t len);
    void sendSerialPacket(uint16_t len);
    void onRadioPacket(MicroBitEvent e);
    void onSerialPacket(MicroBitEvent e);
    void queueTestResponse();

    public:

    PeridoBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b, MicroBitDisplay& display);

    void enable();
};

#endif