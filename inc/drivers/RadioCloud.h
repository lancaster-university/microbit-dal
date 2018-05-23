#ifndef RADIO_CLOUD_H
#define RADIO_CLOUD_H

#include "Radio.h"

class RadioCloud;
struct DataPacket;

#include "RadioREST.h"
#include "RadioCloud.h"


#include "DynamicType.h"
#include "MicroBitConfig.h"

#define CLOUD_HEADER_SIZE                    5
#define MAX_PAYLOAD_SIZE                    (254 - 4 - REST_HEADER_SIZE)

#define REQUEST_TYPE_GET_REQUEST            0x01
#define REQUEST_TYPE_PUT_REQUEST            0x02
#define REQUEST_TYPE_POST_REQUEST           0x04
#define REQUEST_TYPE_DELETE_REQUEST         0x08
#define REQUEST_TYPE_CLOUD_VARIABLE         0x10

#define REQUEST_TYPE_STATUS_ACK             0x20
#define REQUEST_TYPE_STATUS_ERROR           0x40
#define REQUEST_TYPE_STATUS_OK              0x80

#define RADIO_REST_ID                       62965
#define RADIO_CLOUD_VARIABLE_ID             62966

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

class RadioCloud : public MicroBitComponent
{
    Radio& radio;

    DataPacket* txQueue;
    DataPacket* rxQueue;

    int addToQueue(DataPacket** queue, DataPacket* packet);
    DataPacket* removeFromQueue(DataPacket** queue, uint16_t id);

    public:

    uint16_t appId;

    RadioRest& rest;
    RadioVariable& variable;

    RadioCloud(Radio& r, uint16_t appId);

    int addToTxQueue(DataPacket* p);

    DataPacket* removeFromRxQueue(uint16_t id);

    int setAppId(uint16_t id);

    void sendDataPacket(DataPacket* p);

    void packetReceived();

    virtual void idleTick();

    DynamicType recv(uint16_t id);

    DataPacket* recvRaw(uint16_t id);
};


#endif