#ifndef RADIO_CLOUD_H
#define RADIO_CLOUD_H


class Radio;

#include "RadioREST.h"
#include "RadioVariable.h"

#include "DynamicType.h"
#include "MicroBitConfig.h"

#define CLOUD_HEADER_SIZE                    5
#define MAX_PAYLOAD_SIZE                    (254 - 4 - CLOUD_HEADER_SIZE)

#define REQUEST_TYPE_GET_REQUEST            0x01
#define REQUEST_TYPE_POST_REQUEST           0x02
#define REQUEST_TYPE_CLOUD_VARIABLE         0x04
#define REQUEST_TYPE_BROADCAST              0x08

#define REQUEST_STATUS_ACK                  0x20
#define REQUEST_STATUS_ERROR                0x40
#define REQUEST_STATUS_OK                   0x80

#define CLOUD_RADIO_NO_ACK_THRESHOLD        30
#define CLOUD_RADIO_NO_RESPONSE_THRESHOLD   200
#define CLOUD_RADIO_RETRY_THRESHOLD         5

#define DATA_PACKET_WAITING_FOR_SEND        0x01
#define DATA_PACKET_WAITING_FOR_ACK         0x02
#define DATA_PACKET_ACK_RECEIVED            0x04
#define DATA_PACKET_EXPECT_NO_RESPONSE      0x08

#define CLOUD_RADIO_MAXIMUM_BUFFERS         10

#define MICROBIT_RADIO_ID_CLOUD             62964
#define RADIO_REST_ID                       62965
#define RADIO_CLOUD_VARIABLE_ID             62966

#define RADIO_CLOUD_HISTORY_SIZE             4

#define RADIO_CLOUD_STATUS_HUB_MODE         0x02

struct DataPacket
{
    uint16_t id;
    uint16_t app_id;
    uint8_t request_type;
    uint8_t payload[MAX_PAYLOAD_SIZE];

    uint16_t len;
    uint8_t status;
    uint8_t no_response_count;
    uint8_t retry_count;
    DataPacket* next;
} __attribute((packed));

class RadioCloud : public MicroBitComponent
{
    Radio& radio;

    DataPacket* txQueue;
    DataPacket* rxQueue;

    int addToQueue(DataPacket** queue, DataPacket* packet);
    DataPacket* removeFromQueue(DataPacket** queue, uint16_t id);
    DataPacket* peakQueue(DataPacket** queue, uint16_t id);

    bool searchHistory(uint32_t* history, uint16_t app_id, uint16_t id);
    void addToHistory(uint32_t* history, uint8_t* history_index, uint16_t app_id, uint16_t id);

    public:

    uint16_t appId;

    RadioREST rest;
    RadioVariable variable;

    RadioCloud(Radio& r, uint16_t appId);

    int setBridgeMode(bool state);
    bool getBridgeMode();

    int addToTxQueue(DataPacket* p);
    DataPacket* removeFromTxQueue(uint16_t id);
    DataPacket* removeFromRxQueue(uint16_t id);
    DataPacket* peakTxQueue(uint16_t id);

    int setAppId(uint16_t id);

    void sendDataPacket(DataPacket* p);

    void packetReceived();

    virtual void idleTick();

    DynamicType recv(uint16_t id);

    DataPacket* recvRaw(uint16_t id);
};


#endif