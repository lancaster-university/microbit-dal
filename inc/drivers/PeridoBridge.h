#ifndef PERIDO_BRIDGE_H
#define PERIDO_BRIDGE_H

#include "MicroBitPeridoRadio.h"
#include "MicroBitSerial.h"
#include "MicroBitMessageBus.h"
#include "MicroBitDisplay.h"

// the header is only the first two bytes, as the id is placed inside the payload
#define BRIDGE_SERIAL_PACKET_HEADER_SIZE        2

#define PI_STATUS_PASTE_ROW                     3
#define PI_STATUS_PASTE_COLUMN                  0

#define RUNNING_TOGGLE_PASTE_ROW                2
#define RUNNING_TOGGLE_PASTE_COLUMN             4

#define PACKET_COUNT_PASTE_ROW                  4
#define PACKET_COUNT_PASTE_COLUMN               0

#define PACKET_COUNT_BIT_RESOLUTION             5

#define PACKET_COUNT_DISPLAY_RESOLUTION         32 // we only have 5 pixels :)

struct PeridoBridgeSerialPacket
{
    uint8_t app_id;
    uint8_t namespace_id;
    uint16_t request_id;

    uint8_t payload[MICROBIT_PERIDO_MAX_PACKET_SIZE];
}__attribute((packed));

class PeridoBridge
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
    void updatePacketCount();

    public:

    PeridoBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b, MicroBitDisplay& display);

    void enable();
};

#endif