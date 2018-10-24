#include "PeridoRadioCloud.h"
#include "MicroBitPeridoRadio.h"
#include "MicroBitConfig.h"
#include "MicroBitFiber.h"

#define APP_ID_MSK              0xFFFF0000
#define PACKET_ID_MSK           0x0000FFFF

extern void log_string_ch(const char*);
extern void log_num(int);

#define LOG_STRING(str) (log_string_ch(str))
#define LOG_NUM(str) (log_num(str))

// #define LOG_STRING(...) ((void)0)
// #define LOG_NUM(...) ((void)0)

static uint32_t rx_history[RADIO_CLOUD_HISTORY_SIZE] = { 0 };
static uint8_t rx_history_index = 0;

static uint32_t tx_history[RADIO_CLOUD_HISTORY_SIZE] = { 0 };
static uint8_t tx_history_index = 0;

CloudDataItem::~CloudDataItem()
{
    if (packet)
        delete packet;
}

PeridoRadioCloud::PeridoRadioCloud(MicroBitPeridoRadio& r) : radio(r), rest(*this), variable(*this)
{
    this->txQueue = NULL;
    this->rxQueue = NULL;
    rx_history_index = 0;
    tx_history_index = 0;

    fiber_add_idle_component(this);

    // listen for packet received and transmitted events (two separate IDs)
    if (EventModel::defaultEventBus)
        EventModel::defaultEventBus->listen(MICROBIT_ID_RADIO_TX, MICROBIT_EVT_ANY, this, &PeridoRadioCloud::packetTransmitted);
}

int PeridoRadioCloud::setBridgeMode(bool state)
{
    if (state)
    {
        this->status |= RADIO_CLOUD_STATUS_HUB_MODE;
        radio.setAppId(0);
    }
    else
        this->status &= ~RADIO_CLOUD_STATUS_HUB_MODE;

    return MICROBIT_OK;
}

bool PeridoRadioCloud::getBridgeMode()
{
    return (this->status & RADIO_CLOUD_STATUS_HUB_MODE) ? true : false;
}

CloudDataItem* PeridoRadioCloud::removeFromQueue(CloudDataItem** queue, uint16_t id)
{
    if (*queue == NULL)
        return NULL;

    CloudDataItem* ret = NULL;

    CloudDataItem *p = (*queue)->next;
    CloudDataItem *previous = *queue;
    DataPacket* data = (DataPacket*)(*queue)->packet->payload;

    if (id == data->request_id)
    {
        *queue = p;
        ret = previous;
    }
    else
    {
        while (p != NULL)
        {
            data = (DataPacket*)p->packet->payload;

            if (id == data->request_id)
            {
                ret = p;
                previous->next = p->next;
                break;
            }

            previous = p;
            p = p->next;
        }
    }

    return ret;
}

int PeridoRadioCloud::addToQueue(CloudDataItem** queue, CloudDataItem* packet)
{
    int queueDepth = 0;
    packet->next = NULL;

    if (*queue == NULL)
        *queue = packet;
    else
    {
        CloudDataItem *p = *queue;

        while (p->next != NULL)
        {
            p = p->next;
            queueDepth++;
        }

        p->next = packet;
    }

    return MICROBIT_OK;
}

CloudDataItem* PeridoRadioCloud::peakQueuePacketId(CloudDataItem** queue, uint16_t id)
{
    CloudDataItem *p = *queue;

    while (p != NULL)
    {
        if (id == p->packet->id)
            return p;

        p = p->next;
    }

    return NULL;
}

CloudDataItem* PeridoRadioCloud::peakQueue(CloudDataItem** queue, uint16_t id)
{
    CloudDataItem *p = *queue;
    DataPacket *data;

    while (p != NULL)
    {
        data = (DataPacket*)p->packet->payload;
        if (id == data->request_id)
            return p;

        p = p->next;
    }

    return NULL;
}

int PeridoRadioCloud::addToTxQueue(CloudDataItem* p)
{
    return addToQueue(&txQueue, p);
}

CloudDataItem* PeridoRadioCloud::removeFromTxQueue(uint16_t id)
{
    return removeFromQueue(&txQueue, id);
}

CloudDataItem* PeridoRadioCloud::removeFromRxQueue(uint16_t id)
{
    return removeFromQueue(&rxQueue, id);
}

CloudDataItem* PeridoRadioCloud::peakTxQueue(uint16_t id)
{
    return peakQueue(&txQueue, id);
}

void PeridoRadioCloud::sendCloudDataItem(CloudDataItem* p)
{
    radio.send(p->packet);
}

void PeridoRadioCloud::packetTransmitted(MicroBitEvent evt)
{
    uint16_t id = evt.value;

    CloudDataItem* p = peakQueuePacketId(&txQueue, id);
    DataPacket* t = (DataPacket*)p->packet->payload;

    // if it's broadcast just delete from the tx queue once it has been sent
    if (p)
    {
        LOG_STRING("PACKET_TRANS");
        if ((t->request_type & REQUEST_TYPE_BROADCAST || p->status & DATA_PACKET_EXPECT_NO_RESPONSE))
        {
            CloudDataItem *c = removeFromQueue(&txQueue, t->request_id);
            delete c;
            return;
        }

        // evt.value... get packet and flag correctly.
        // packet transmitted, flag as waiting for ACK.
        p->no_response_count = 0;
        p->retry_count = 0;

        p->status |= (DATA_PACKET_WAITING_FOR_ACK);
    }
}

int PeridoRadioCloud::send(uint8_t request_type, uint8_t* buffer, int len)
{
    CloudDataItem* c = new CloudDataItem;
    PeridoFrameBuffer* buf = new PeridoFrameBuffer;

    LOG_STRING("SEND");
    buf->id = radio.generateId(radio.getAppId(), 0);
    buf->length = len + (MICROBIT_PERIDO_HEADER_SIZE - 1) + CLOUD_HEADER_SIZE; // add 1 for request type
    buf->app_id = radio.getAppId();
    buf->namespace_id = 0;
    buf->ttl = 4;
    buf->initial_ttl = 4;
    buf->time_since_wake = 0;
    buf->period = 0;

    DataPacket* data = (DataPacket*)buf->payload;
    data->request_id = generateId(); //  try to go for uniqueness within the driver.
    data->request_type = request_type;
    memcpy(data->payload, buffer, len);

    c->status = DATA_PACKET_WAITING_FOR_SEND;
    c->packet = buf;

    LOG_STRING("ADDING TO Q");

    addToTxQueue(c);

    return data->request_id;
}

int PeridoRadioCloud::sendAck(uint16_t request_id, uint8_t app_id, uint8_t namespace_id)
{
    LOG_STRING("SEND_ACK");
    PeridoFrameBuffer buf;

    buf.id = radio.generateId(app_id, namespace_id);
    LOG_STRING("GENNED ID");
    buf.length = 0 + (MICROBIT_PERIDO_HEADER_SIZE - 1) + CLOUD_HEADER_SIZE;
    buf.app_id = app_id;
    buf.namespace_id = namespace_id;
    buf.ttl = 4;
    buf.initial_ttl = 4;
    buf.time_since_wake = 0;
    buf.period = 0;

    DataPacket* data = (DataPacket*)buf.payload;
    data->request_id = request_id;
    data->request_type = REQUEST_STATUS_ACK;

    radio.send(&buf);
    LOG_STRING("OUT");
    return MICROBIT_OK;
}

void PeridoRadioCloud::idleTick()
{
    if (txQueue == NULL)
        return;

    // walk the txqueue and check to see if any have exceeded our retransmit time
    bool transmitted = false;

    CloudDataItem* p = txQueue;
    while(p != NULL)
    {
        // only transmit once every idle tick
        if (p->status & DATA_PACKET_WAITING_FOR_SEND && !transmitted)
        {
            transmitted = true;
            sendCloudDataItem(p);
            p->status &=~(DATA_PACKET_WAITING_FOR_SEND);
        }
        else if (p->status & DATA_PACKET_WAITING_FOR_ACK)
        {
             p->no_response_count++;

            if (p->no_response_count > CLOUD_RADIO_NO_ACK_THRESHOLD && !transmitted)
            {
                p->retry_count++;

                // if we've exceeded our retry threshold, we  break and remove from the tx queue, flagging an error
                if (p->retry_count > CLOUD_RADIO_RETRY_THRESHOLD)
                {
                    LOG_STRING("ACK TIMEOUT");
                    break;
                }

                LOG_STRING("RESEND");

                // resend
                sendCloudDataItem(p);
                p->no_response_count = 0;
                transmitted = true;
            }
        }
        // if we've received an ack, time out if we never actually receive a reply.
        else if (p->status & DATA_PACKET_ACK_RECEIVED)
        {
            p->no_response_count++;

            if (p->no_response_count > CLOUD_RADIO_NO_RESPONSE_THRESHOLD || p->status & DATA_PACKET_EXPECT_NO_RESPONSE)
            {
                LOG_STRING("RESPONSE TIMEOUT");
                break;
            }
        }

        p = p->next;
    }

    // if we've iterated over the full list without a break, p will be null.
    if (p)
    {
        DataPacket *t = (DataPacket*)p->packet->payload;
        CloudDataItem *c = removeFromQueue(&txQueue, t->request_id);

        if (c)
        {
            if (c->status & DATA_PACKET_EXPECT_NO_RESPONSE)
            {
                delete c;
                return;
            }

            uint8_t rt = t->request_type;
            t->request_type = REQUEST_STATUS_ERROR;

            // expect client code to check for errors...
            addToQueue(&rxQueue, c);

            if (rt & (REQUEST_TYPE_GET_REQUEST | REQUEST_TYPE_POST_REQUEST))
                rest.handleTimeout(t->request_id);

            if (rt & REQUEST_TYPE_CLOUD_VARIABLE)
                variable.handleTimeout(t->request_id);
        }
    }
}

bool PeridoRadioCloud::searchHistory(uint32_t* history, uint16_t request_id, uint8_t app_id, uint8_t namespace_id)
{
    uint32_t combined_id = (request_id << 16) | (app_id << 8) | namespace_id;
    for (int idx = 0; idx < RADIO_CLOUD_HISTORY_SIZE; idx++)
    {
        if (history[idx] == combined_id)
            return true;
    }

    return false;
}

void PeridoRadioCloud::addToHistory(uint32_t* history, uint8_t* history_index, uint16_t request_id, uint8_t app_id, uint8_t namespace_id)
{
    history[*history_index] = (request_id << 16) | (app_id << 8) | namespace_id;
    *history_index = (*history_index + 1) % RADIO_CLOUD_HISTORY_SIZE;
}

/**
  * Protocol handler callback. This is called when the radio receives a packet marked as a datagram.
  *
  * This function process this packet, and queues it for user reception.
  */
void PeridoRadioCloud::packetReceived()
{
    LOG_STRING("PKT!");
    PeridoFrameBuffer* packet = radio.recv();
    DataPacket* temp = (DataPacket*)packet->payload;

    // LOG_NUM(packet->length);
    // for (int i = 0; i < packet->length - (MICROBIT_PERIDO_HEADER_SIZE - 1); i++)
    //     LOG_NUM(packet->payload[i]);

    // first we check if we've received an ack
    if (temp->request_type & REQUEST_STATUS_ACK)
    {
        LOG_STRING("ACK!");
        CloudDataItem* p = peakTxQueue(temp->request_id);

        if (p)
        {
            // flag this packet as receiving the ack.
            // once we do this the idle tick will remove the packet for us.
            p->status &= ~(DATA_PACKET_WAITING_FOR_ACK);
            p->status |= DATA_PACKET_ACK_RECEIVED;
            p->no_response_count = 0;
            p->retry_count = 0;
        }

        // delete the ack packet and return
        delete packet;
        return;
    }

    // check if we are the originator of the request or a bridge
    // if we are, send an ack
    if (peakTxQueue(temp->request_id) || getBridgeMode())
    {
        LOG_STRING("OUR TX");
        sendAck(temp->request_id, packet->app_id, packet->namespace_id);
    }

    // check if we've already received the packet:
    if (searchHistory(rx_history, temp->request_id, packet->app_id, packet->namespace_id))
    {
        LOG_STRING("ALREADY RX'd");
        delete packet;
        return;
    }

    addToHistory(rx_history, &rx_history_index, temp->request_id, packet->app_id, packet->namespace_id);
    LOG_STRING("ADDING");

    // we have received a response, remove any matching packets from the txQueue
    CloudDataItem* p = removeFromQueue(&txQueue, temp->request_id);

    // if we're a bridge, we receive all packets allocate a new data item for rx (unless we sent a packet).
    if (getBridgeMode() && p == NULL)
    {
        LOG_STRING("NEW BUF");
        p = new CloudDataItem;
    }
    // if it's in our tx queue and we're either a bridge or a normal device
    // delete the previously held packet used for tx.
    else if (p)
    {
        delete p->packet;
        LOG_STRING("DELETE OLD");
    }
    // otherwise, if it's not in our txqueue, so we shan't store it.
    else
    {
        LOG_STRING("IGNORE, delete packet");
        delete packet;
        return;
    }

    // add to our RX queue for app handling.
    p->packet = packet;
    p->status = 0;
    p->no_response_count = 0;
    p->retry_count = 0;
    p->next = NULL;

    LOG_STRING("ADDING TO RX Q");
    addToQueue(&rxQueue, p);


    LOG_NUM(temp->request_id);
    LOG_STRING("STATUS: ");
    LOG_NUM(p->status);

    // interception event for Bridge.cpp
    MicroBitEvent(MICROBIT_RADIO_ID_CLOUD, temp->request_id);

    // used for notifying underlying services that a packet has been received.
    // this occurs after the packet has been added to the rxQueue.
    if (temp->request_type & (REQUEST_TYPE_GET_REQUEST | REQUEST_TYPE_POST_REQUEST))
        rest.handlePacket(temp->request_id);

    if (temp->request_type & REQUEST_TYPE_CLOUD_VARIABLE)
        variable.handlePacket(temp->request_id);
}

extern void log_string(char c);
DynamicType PeridoRadioCloud::recv(uint16_t id)
{
    CloudDataItem *c = removeFromRxQueue(id);
    DataPacket* t = (DataPacket*)c->packet->payload;

    if (c == NULL)
    {
        LOG_STRING("recv NULL");
        return DynamicType();
    }

    DynamicType dt;

    if (t->request_type & REQUEST_STATUS_ERROR)
    {
        LOG_STRING("recv ERR");
        dt = DynamicType(7, (uint8_t*)"\01ERROR\0", DYNAMIC_TYPE_STATUS_ERROR);
    }
    else
    {
        LOG_STRING("recv OK");
        dt = DynamicType(c->packet->length - (MICROBIT_PERIDO_HEADER_SIZE - 1) - CLOUD_HEADER_SIZE, t->payload, 0);
    }

    delete c;

    return dt;
}

CloudDataItem* PeridoRadioCloud::recvRaw(uint16_t id)
{
    return removeFromRxQueue(id);
}

CloudDataItem* PeridoRadioCloud::recvRaw()
{
    CloudDataItem *p = rxQueue;
    DataPacket* dp = (DataPacket*)p->packet->payload;

    if (p)
        return removeFromRxQueue(dp->request_id);

    return NULL;
}

uint16_t PeridoRadioCloud::generateId()
{
    LOG_STRING("GEN ID:");
    uint16_t new_id;
    bool seenBefore = true;

    while (seenBefore)
    {
        new_id = microbit_random(65535);
        seenBefore = false;

        CloudDataItem* txItem = peakQueue(&txQueue, new_id);
        CloudDataItem* rxItem = peakQueue(&rxQueue, new_id);

        if (txItem || rxItem)
            seenBefore = true;
    }
    LOG_NUM(new_id);

    return new_id;

}

