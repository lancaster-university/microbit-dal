#ifndef RADIO_REST_H
#define RADIO_REST_H

#include "Radio.h"
#include "DynamicType.h"
#include "MicroBitConfig.h"
#include "RadioCloud.h"

#define REST_RADIO_MAXIMUM_TX_BUFFERS       10
#define REST_RADIO_MAXIMUM_RX_BUFFERS       10

#define REST_RADIO_NO_ACK_THRESHOLD         30
#define REST_RADIO_NO_RESPONSE_THRESHOLD    100
#define REST_RADIO_RETRY_THRESHOLD          5

#define DATA_PACKET_WAITING_FOR_SEND        0x01
#define DATA_PACKET_WAITING_FOR_ACK         0x02
#define DATA_PACKET_ACK_RECEIVED            0x04


class RadioREST : public MicroBitComponent
{
    RadioCloud& cloud;

    DataPacket* composePacket(uint8_t type, uint8_t* payload, uint8_t payload_len, uint16_t app_id, uint16_t packet_id = 0);

    public:

    RadioREST(RadioCloud& r, uint16_t appId);

    DynamicType getRequest(ManagedString url);

    uint16_t getRequestAsync(ManagedString url);

    DynamicType postRequest(ManagedString url, DynamicType& parameters);

    uint16_t postRequestAsync(ManagedString url, DynamicType& parameters);

    void handlePacket();

    DynamicType recv(uint16_t id);

    DataPacket* recvRaw(uint16_t id);
};


#endif