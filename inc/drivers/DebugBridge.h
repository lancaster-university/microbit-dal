#ifndef DEBUG_BRIDGE_H
#define DEBUG_BRIDGE_H

#include "MicroBitPeridoRadio.h"
#include "MicroBitSerial.h"
#include "MicroBitMessageBus.h"
#include "MicroBitDisplay.h"
#include "MicroBitComponent.h"

// the header is only the first two bytes, as the id is placed inside the payload

class DebugBridge : public MicroBitComponent
{
    MicroBitPeridoRadio& radio;
    MicroBitSerial& serial;
    MicroBitDisplay& display;

    uint32_t packetCount;

    void onRadioPacket(MicroBitEvent e);

    public:

    DebugBridge(MicroBitPeridoRadio& r, MicroBitSerial& s, MicroBitMessageBus& b, MicroBitDisplay& display);

    void enable();
};

#endif