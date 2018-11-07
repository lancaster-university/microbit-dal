#ifndef PERIDO_RADIO_CLOUD_H
#define PERIDO_RADIO_CLOUD_H


struct PeridoFrameBuffer;
class MicroBitPeridoRadio;

#include "PeridoRadioREST.h"
#include "PeridoRadioVariable.h"

#include "DynamicType.h"
#include "MicroBitConfig.h"
#include "MicroBitEvent.h"

#define CLOUD_HEADER_SIZE                    3
#define MAX_PAYLOAD_SIZE                    (255 - 10 - CLOUD_HEADER_SIZE)

#define REQUEST_TYPE_GET_REQUEST            0x01
#define REQUEST_TYPE_POST_REQUEST           0x02
#define REQUEST_TYPE_CLOUD_VARIABLE         0x04
#define REQUEST_TYPE_BROADCAST              0x08

#define REQUEST_TYPE_HELLO                  0x10
#define REQUEST_STATUS_ACK                  0x20
#define REQUEST_STATUS_ERROR                0x40
#define REQUEST_STATUS_OK                   0x80

#define CLOUD_RADIO_NO_ACK_THRESHOLD        334   // 334 * 6 (scheduling quantum) == 2000 ms
#define CLOUD_RADIO_NO_RESPONSE_THRESHOLD   668   // 668 * 6 (scheduling quantum) == 4000 ms
#define CLOUD_RADIO_RETRY_THRESHOLD         2    // resend packet, equates to 3 resends...

#define DATA_PACKET_WAITING_FOR_SEND        0x01
#define DATA_PACKET_AWAITING_RESPONSE       0x02
#define DATA_PACKET_EXPECT_NO_RESPONSE      0x08

#define CLOUD_RADIO_MAXIMUM_BUFFERS         10

#define MICROBIT_RADIO_ID_CLOUD             62964
#define RADIO_REST_ID                       62965
#define RADIO_CLOUD_VARIABLE_ID             62966

#define RADIO_CLOUD_TX_THROTTLE             12
#define RADIO_CLOUD_HISTORY_SIZE             20

#define RADIO_CLOUD_STATUS_HUB_MODE         0x02

struct CloudDataItem
{
    PeridoFrameBuffer* packet;
    uint8_t status;
    uint8_t retry_count;
    uint16_t no_response_count;
    CloudDataItem* next;

    ~CloudDataItem();
};

struct DataPacket
{
    uint16_t request_id;
    uint8_t request_type;
    uint8_t payload[MAX_PAYLOAD_SIZE];

} __attribute((packed));

class PeridoRadioCloud : public MicroBitComponent
{
    MicroBitPeridoRadio& radio;
    uint8_t namespaceId;

    CloudDataItem* txQueue;
    CloudDataItem* rxQueue;

    int addToQueue(CloudDataItem** queue, CloudDataItem* packet);
    CloudDataItem* removeFromQueue(CloudDataItem** queue, uint16_t id);
    CloudDataItem* peakQueue(CloudDataItem** queue, uint16_t id);
    CloudDataItem* peakQueuePacketId(CloudDataItem** queue, uint16_t id);

    void handleError(DataPacket* t);

    bool searchHistory(uint32_t* history, uint16_t request_id, uint8_t app_id, uint8_t namespace_id);
    void addToHistory(uint32_t* history, uint8_t* history_index, uint16_t request_id, uint8_t app_id, uint8_t namespace_id);

    public:

    PeridoRadioREST rest;
    PeridoRadioVariable variable;

    PeridoRadioCloud(MicroBitPeridoRadio& r, uint8_t namespaceId);

    int setBridgeMode(bool state);
    bool getBridgeMode();

    int addToTxQueue(CloudDataItem* p);
    CloudDataItem* removeFromTxQueue(uint16_t id);
    CloudDataItem* removeFromRxQueue(uint16_t id);
    CloudDataItem* peakTxQueue(uint16_t id);

    int sendCloudDataItem(CloudDataItem* p);

    int send(uint8_t request_type, uint8_t* buffer, int len);

    void packetReceived();
    void packetTransmitted(uint16_t id);

    virtual void systemTick();

    DynamicType recv(uint16_t id);

    CloudDataItem* recvRaw(uint16_t id);

    CloudDataItem* recvRaw();

    uint8_t getNamespaceId();

    uint16_t generateId();
};


#endif