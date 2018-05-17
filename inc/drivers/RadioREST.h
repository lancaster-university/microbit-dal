#ifndef RADIO_REST_H
#define RADIO_REST_H

#include "Radio.h"
#include "DynamicType.h"
#include "MicroBitConfig.h"

#define RADIO_REST_ID                       62965
#define REST_HEADER_SIZE                    5
#define REST_RADIO_MAXIMUM_TX_BUFFERS       10
#define REST_RADIO_MAXIMUM_RX_BUFFERS       10
#define MAX_PAYLOAD_SIZE                    (254 - 4 - REST_HEADER_SIZE)

#define REST_RADIO_NO_ACK_THRESHOLD         30
#define REST_RADIO_NO_RESPONSE_THRESHOLD    100
#define REST_RADIO_RETRY_THRESHOLD          5

#define REQUEST_TYPE_GET_REQUEST            0x01
#define REQUEST_TYPE_PUT_REQUEST            0x02
#define REQUEST_TYPE_POST_REQUEST           0x04
#define REQUEST_TYPE_DELETE_REQUEST         0x08

#define REQUEST_TYPE_STATUS_ACK             0x10
#define REQUEST_TYPE_STATUS_ERROR           0x20
#define REQUEST_TYPE_STATUS_OK              0x40
#define REQUEST_TYPE_REPEATING              0x80

#define DATA_PACKET_WAITING_FOR_SEND        0x01
#define DATA_PACKET_WAITING_FOR_ACK         0x02
#define DATA_PACKET_ACK_RECEIVED            0x04

struct DataPacket
{
    uint16_t id;
    uint16_t app_id;
    uint8_t request_type;
    uint8_t payload[MAX_PAYLOAD_SIZE];

    uint16_t len;
    uint8_t status;
    int8_t no_response_count;
    int8_t retry_count;
    DataPacket* next;
} __attribute((packed));

class RadioREST : public MicroBitComponent
{
    uint16_t appId;
    Radio& radio;

    DataPacket* txQueue;
    DataPacket* rxQueue;

    int addToQueue(DataPacket** queue, DataPacket* packet);
    DataPacket* removeFromQueue(DataPacket** queue, uint16_t id);
    DataPacket* composePacket(uint8_t type, uint8_t* payload, uint8_t payload_len, uint16_t app_id, uint16_t packet_id = 0);
    void sendDataPacket(DataPacket* p);

    public:

    RadioREST(Radio& r, uint16_t appId);

    int send(DataPacket* p);

    DynamicType getRequest(ManagedString url, bool repeating = false);

    uint16_t getRequestAsync(ManagedString url, bool repeating = false);

    DynamicType postRequest(ManagedString url, DynamicType& parameters);

    uint16_t postRequestAsync(ManagedString url, DynamicType& parameters);

    void packetReceived();

    virtual void idleTick();

    DynamicType recv(uint16_t id);

    DataPacket* recvRaw(uint16_t id);
};


#endif