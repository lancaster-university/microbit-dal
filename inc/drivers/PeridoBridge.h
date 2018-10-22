#ifndef PERIDO_BRIDGE_H
#define PERIDO_BRIDGE_H

#include "MicroBitPeridoRadio.h"
#include "MicroBitSerial.h"
#include "MicroBitMessageBus.h"

// the header is only the first two bytes, as the id is placed inside the payload
#define BRIDGE_SERIAL_PACKET_HEADER_SIZE        2

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

    PeridoBridgeSerialPacket serialPacket; // holds data to be transmitted or received

    void sendHelloPacket();
    void sendSerialPacket(uint16_t len);
    void onRadioPacket(MicroBitEvent e);
    void onSerialPacket(MicroBitEvent e);

    public:

    PeridoBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b);
};

#endif