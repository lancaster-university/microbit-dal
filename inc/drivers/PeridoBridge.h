#ifndef PERIDO_BRIDGE_H
#define PERIDO_BRIDGE_H

#include "MicroBitPeridoRadio.h"
#include "MicroBitSerial.h"
#include "MicroBitMessageBus.h"

#define BRIDGE_SERIAL_PACKET_HEADER_SIZE        4

struct PeridoBridgeSerialPacket
{
    uint8_t app_id;
    uint8_t namespace_id;
    uint16_t id;

    uint8_t payload[MICROBIT_PERIDO_MAX_PACKET_SIZE];
}__attribute((packed));

class PeridoBridge
{
    MicroBitPeridoRadio& radio;
    MicroBitSerial& serial;

    PeridoBridgeSerialPacket serialPacket; // holds data to be transmitted or received

    void onRadioPacket(MicroBitEvent e);
    void onSerialPacket(MicroBitEvent e);

    bool searchHistory(uint16_t app_id, uint16_t id);
    void addToHistory(uint16_t app_id, uint16_t id);

    void test_send(DataPacket* p);

    public:

    PeridoBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b);
};

#endif