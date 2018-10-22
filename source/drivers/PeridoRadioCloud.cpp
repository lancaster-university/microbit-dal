#include "PeridoRadioCloud.h"
#include "MicroBitPeridoRadio.h"
#include "MicroBitConfig.h"
#include "MicroBitFiber.h"

#define APP_ID_MSK              0xFFFF0000
#define PACKET_ID_MSK           0x0000FFFF

static uint32_t rx_history[RADIO_CLOUD_HISTORY_SIZE] = { 0 };
static uint8_t rx_history_index = 0;

static uint32_t tx_history[RADIO_CLOUD_HISTORY_SIZE] = { 0 };
static uint8_t tx_history_index = 0;

CloudDataItem::~CloudDataItem()
{
#warning check this destructor is called when deleting a CloudDataItem
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

    if (id == (*queue)->packet->id)
    {
        *queue = p;
        ret = previous;
    }
    else
    {
        while (p != NULL)
        {
            if (id == p->packet->id)
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

CloudDataItem* PeridoRadioCloud::peakQueue(CloudDataItem** queue, uint16_t id)
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

extern void change_display();
extern void log_string_ch(const char*);
void PeridoRadioCloud::packetTransmitted(MicroBitEvent evt)
{
    uint16_t id = evt.value;

    CloudDataItem* p = peakTxQueue(id);
    DataPacket* t = (DataPacket*)p->packet->payload;

    // if it's broadcast just delete from the tx queue once it has been sent
    if (p)
    {
        if ((t->request_type & REQUEST_TYPE_BROADCAST || p->status & DATA_PACKET_EXPECT_NO_RESPONSE))
        {
            CloudDataItem *c = removeFromQueue(&txQueue, id);
            delete c;
            return;
        }

        // evt.value... get packet and flag correctly.
        // packet transmitted, flag as waiting for ACK.
        p->no_response_count = 0;
        p->retry_count = 0;

        p->status |= (DATA_PACKET_WAITING_FOR_ACK);

        change_display();
    }
}

int PeridoRadioCloud::send(uint8_t request_type, uint8_t* buffer, int len)
{
    CloudDataItem* c = new CloudDataItem;
    PeridoFrameBuffer* buf = new PeridoFrameBuffer;

    buf->id = microbit_random(65535); // perhaps we should spin here, and try to go for uniqueness within the driver.
    buf->length = len + (MICROBIT_PERIDO_HEADER_SIZE - 1) + CLOUD_HEADER_SIZE; // add 1 for request type
    buf->app_id = radio.getAppId();
    buf->namespace_id = 0;
    buf->ttl = 4;
    buf->initial_ttl = 4;
    buf->time_since_wake = 0;
    buf->period = 0;

    DataPacket* data = (DataPacket*)buf->payload;
    data->request_type = request_type;
    memcpy(data->payload, buffer, len);

    c->status = DATA_PACKET_WAITING_FOR_SEND;
    c->packet = buf;

    addToTxQueue(c);

    return buf->id;
}

int PeridoRadioCloud::sendAck(uint16_t id, uint8_t app_id, uint8_t namespace_id)
{
    CloudDataItem* ack = new CloudDataItem;
    PeridoFrameBuffer* buf = new PeridoFrameBuffer;

    buf->id = id;
    buf->length = 0 + MICROBIT_PERIDO_HEADER_SIZE - 1;
    buf->app_id = app_id;
    buf->namespace_id = namespace_id;
    buf->ttl = 4;
    buf->initial_ttl = 4;
    buf->time_since_wake = 0;
    buf->period = 0;

    DataPacket* data = (DataPacket*)buf->payload;
    data->request_type = REQUEST_STATUS_ACK;

    ack->status = 0;
    ack->packet = buf;

    sendCloudDataItem(ack);
    delete ack;
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
                    break;
                log_string_ch("RESEND");
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
                break;
        }

        p = p->next;
    }

    // if we've iterated over the full list without a break, p will be null.
    if (p)
    {
        CloudDataItem *c = removeFromQueue(&txQueue, p->packet->id);
        DataPacket *t = (DataPacket*)c->packet->payload;
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
                rest.handleTimeout(c->packet->id);

            if (rt & REQUEST_TYPE_CLOUD_VARIABLE)
                variable.handleTimeout(c->packet->id);
        }
    }
}

bool PeridoRadioCloud::searchHistory(uint32_t* history, uint16_t app_id, uint16_t id)
{
    for (int idx = 0; idx < RADIO_CLOUD_HISTORY_SIZE; idx++)
    {
        if (((history[idx] & APP_ID_MSK) >> 16) == app_id && (history[idx] & PACKET_ID_MSK) == id)
            return true;
    }

    return false;
}

void PeridoRadioCloud::addToHistory(uint32_t* history, uint8_t* history_index, uint16_t app_id, uint16_t id)
{
    history[*history_index] = ((app_id << 16) | id);
    *history_index = (*history_index + 1) % RADIO_CLOUD_HISTORY_SIZE;
}

/**
  * Protocol handler callback. This is called when the radio receives a packet marked as a datagram.
  *
  * This function process this packet, and queues it for user reception.
  */

extern void log_string_ch(const char*);
extern void log_num(int);
void PeridoRadioCloud::packetReceived()
{
    log_string_ch("PKT!");
    PeridoFrameBuffer* packet = radio.recv();
    DataPacket* temp = (DataPacket*)packet->payload;

    log_num(packet->length);
    for (int i = 0; i < packet->length - (MICROBIT_PERIDO_HEADER_SIZE - 1); i++)
        log_num(packet->payload[i]);

    // if an ack, update our packet status
    if (temp->request_type & REQUEST_STATUS_ACK)
    {
        CloudDataItem* p = peakTxQueue(packet->id);

        // we don't expect a response from a node here.
        if (getBridgeMode() && p)
        {
            p = removeFromTxQueue(packet->id);
            if (p == NULL)
                while(1);
            delete p;
        }
        else if (p)
        {
            // otherwise in normal mode, we expect a response from a bridged ubit
            p->status &= ~(DATA_PACKET_WAITING_FOR_ACK);
            p->status |= DATA_PACKET_ACK_RECEIVED;
            p->no_response_count = 0;
            p->retry_count = 0;
        }

        delete packet;
        return;
    }

    if (!getBridgeMode())
    {
        // ack any packets we've previously sent
        if (searchHistory(tx_history, packet->app_id, packet->id))
            sendAck(packet->id, packet->app_id, packet->namespace_id);

        // ignore previously received packets
        if (searchHistory(rx_history, packet->app_id, packet->id))
        {
            delete packet;
            return;
        }

        addToHistory(rx_history, &rx_history_index, packet->app_id, packet->id);
    }

    // we have received a response, remove any matching packets from the txQueue
    CloudDataItem* toDelete = removeFromQueue(&txQueue, packet->id);

    if (toDelete)
        delete toDelete;

    // add to our RX queue for app handling.
    CloudDataItem* p = new CloudDataItem;
    p->packet = packet;
    p->status = 0;
    p->no_response_count = 0;
    p->retry_count = 0;
    p->next = NULL;

    addToQueue(&rxQueue, p);

    // interception event for Bridge.cpp
    MicroBitEvent(MICROBIT_RADIO_ID_CLOUD, p->packet->id);

    // used for notifying underlying services that a packet has been received.
    // this occurs after the packet has been added to the rxQueue.
    if (temp->request_type & (REQUEST_TYPE_GET_REQUEST | REQUEST_TYPE_POST_REQUEST))
        rest.handlePacket(p->packet->id);

    if (temp->request_type & REQUEST_TYPE_CLOUD_VARIABLE)
        variable.handlePacket(p->packet->id);
}


DynamicType PeridoRadioCloud::recv(uint16_t id)
{
    CloudDataItem *c = removeFromRxQueue(id);
    DataPacket* t = (DataPacket*)c->packet->payload;

    if (t == NULL)
        return DynamicType();

    DynamicType dt;

    if (t->request_type & REQUEST_STATUS_ERROR)
        dt = DynamicType(7, (uint8_t*)"\01ERROR\0", DYNAMIC_TYPE_STATUS_ERROR);
    else
#warning potentially bad change here
        dt = DynamicType(c->packet->length - MICROBIT_PERIDO_HEADER_SIZE - CLOUD_HEADER_SIZE, t->payload, 0);

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

    if (p)
        return removeFromRxQueue(p->packet->id);

    return NULL;
}