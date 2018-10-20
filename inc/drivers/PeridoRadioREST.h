#ifndef PERIDO_RADIO_REST_H
#define PERIDO_RADIO_REST_H

struct CloudDataItem;
class PeridoRadioCloud;

#include "DynamicType.h"
#include "MicroBitConfig.h"

class PeridoRadioREST
{
    PeridoRadioCloud& cloud;

    public:

    PeridoRadioREST(PeridoRadioCloud& r);

    DynamicType getRequest(ManagedString url);

    uint16_t getRequestAsync(ManagedString url);

    DynamicType postRequest(ManagedString url, DynamicType& parameters);

    uint16_t postRequestAsync(ManagedString url, DynamicType& parameters);

    void handlePacket(uint16_t id);
    void handleTimeout(uint16_t id);

    DynamicType recv(uint16_t id);

    CloudDataItem* recvRaw(uint16_t id);
};


#endif