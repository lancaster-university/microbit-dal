#include "RadioCloud.h"
#include "Radio.h"
#include "MicroBitFiber.h"

#define APP_ID_MSK              0xFFFF0000
#define PACKET_ID_MSK           0x0000FFFF

static uint32_t rx_history[RADIO_CLOUD_HISTORY_SIZE] = { 0 };
static uint8_t rx_history_index = 0;

static uint32_t tx_history[RADIO_CLOUD_HISTORY_SIZE] = { 0 };
static uint8_t tx_history_index = 0;

RadioCloud::RadioCloud(Radio& r, uint16_t app_id) : radio(r), rest(*this), variable(*this)
{
    this->appId = app_id;
    this->txQueue = NULL;
    this->rxQueue = NULL;
    rx_history_index = 0;
    tx_history_index = 0;

    fiber_add_idle_component(this);
}

int RadioCloud::setBridgeMode(bool state)
{
    if (state)
    {
        this->status |= RADIO_CLOUD_STATUS_HUB_MODE;
        setAppId(0);
    }
    else
        this->status &= ~RADIO_CLOUD_STATUS_HUB_MODE;

    return MICROBIT_OK;
}

bool RadioCloud::getBridgeMode()
{
    return (this->status & RADIO_CLOUD_STATUS_HUB_MODE) ? true : false;
}

DataPacket* RadioCloud::removeFromQueue(DataPacket** queue, uint16_t id)
{
    if (*queue == NULL)
		return NULL;

    DataPacket* ret = NULL;

    __disable_irq();
	DataPacket *p = (*queue)->next;
	DataPacket *previous = *queue;

	if (id == (*queue)->id)
	{
		*queue = p;
		ret = previous;
	}
    else
    {
        while (p != NULL)
        {
            if (id == p->id)
            {
                ret = p;
                previous->next = p->next;
                break;
            }

            previous = p;
            p = p->next;
        }
    }

    __enable_irq();

	return ret;
}

int RadioCloud::addToQueue(DataPacket** queue, DataPacket* packet)
{
    int queueDepth = 0;
    packet->next = NULL;

    __disable_irq();
    if (*queue == NULL)
        *queue = packet;
    else
    {
        DataPacket *p = *queue;

        while (p->next != NULL)
        {
            p = p->next;
            queueDepth++;
        }

        if (queueDepth >= CLOUD_RADIO_MAXIMUM_BUFFERS)
        {
            delete packet;
            return MICROBIT_NO_RESOURCES;
        }

        p->next = packet;
    }
    __enable_irq();

    return MICROBIT_OK;
}

DataPacket* RadioCloud::peakQueue(DataPacket** queue, uint16_t id)
{
    DataPacket *p = *queue;

    __disable_irq();
    while (p != NULL)
    {
        if (id == p->id)
            return p;

        p = p->next;
    }
    __enable_irq();

    return NULL;
}

int RadioCloud::addToTxQueue(DataPacket* p)
{
    return addToQueue(&txQueue, p);
}

DataPacket* RadioCloud::removeFromTxQueue(uint16_t id)
{
    return removeFromQueue(&txQueue, id);
}

DataPacket* RadioCloud::removeFromRxQueue(uint16_t id)
{
    return removeFromQueue(&rxQueue, id);
}

DataPacket* RadioCloud::peakTxQueue(uint16_t id)
{
    return peakQueue(&txQueue, id);
}

int RadioCloud::setAppId(uint16_t id)
{
    this->appId = id;
    return MICROBIT_OK;
}

void RadioCloud::sendDataPacket(DataPacket* p)
{
    RadioFrameBuffer buf;

    buf.length = p->len + CLOUD_HEADER_SIZE + MICROBIT_RADIO_HEADER_SIZE - 1;
    buf.version = 1;
    buf.group = 0;
    buf.protocol = MICROBIT_RADIO_PROTOCOL_CLOUD;
    memcpy(buf.payload, (uint8_t*)p, p->len + CLOUD_HEADER_SIZE);

    radio.send(&buf);
}

void RadioCloud::idleTick()
{
    if (txQueue == NULL)
        return;

    // walk the txqueue and check to see if any have exceeded our retransmit time
    bool transmitted = false;

    DataPacket* p = txQueue;
    while(p != NULL)
    {
        // only transmit once every idle tick
        if (p->status & DATA_PACKET_WAITING_FOR_SEND && !transmitted)
        {
            transmitted = true;
            sendDataPacket(p);
            addToHistory(tx_history, &tx_history_index, p->app_id, p->id);
            p->status &=~(DATA_PACKET_WAITING_FOR_SEND);

            if (p->request_type & REQUEST_TYPE_BROADCAST)
                break;

            p->status |= DATA_PACKET_WAITING_FOR_ACK;
            p->no_response_count = 0;
            p->retry_count = 0;
        }
        else if (p->status & DATA_PACKET_WAITING_FOR_ACK)
        {
             p->no_response_count++;

            if (p->no_response_count > CLOUD_RADIO_NO_ACK_THRESHOLD && !transmitted)
            {
                p->retry_count++;

                if (p->retry_count > CLOUD_RADIO_RETRY_THRESHOLD)
                    break;

                sendDataPacket(p);
                p->no_response_count = 0;
                transmitted = true;
            }
        }
        else if (p->status & DATA_PACKET_ACK_RECEIVED)
        {
            p->no_response_count++;

            if (p->no_response_count > CLOUD_RADIO_NO_RESPONSE_THRESHOLD || p->status & DATA_PACKET_EXPECT_NO_RESPONSE)
                break;
        }

        p = p->next;
    }

    if (p)
    {
        DataPacket *t = removeFromQueue(&txQueue, p->id);

        uint8_t rt = t->request_type;
        t->request_type = REQUEST_STATUS_ERROR;

        // expect client code to check for errors...
        addToQueue(&rxQueue, t);

        if (rt & (REQUEST_TYPE_GET_REQUEST | REQUEST_TYPE_POST_REQUEST))
            rest.handleTimeout(t->id);

        if (rt & REQUEST_TYPE_CLOUD_VARIABLE)
            variable.handleTimeout(t->id);
    }
}

bool RadioCloud::searchHistory(uint32_t* history, uint16_t app_id, uint16_t id)
{
    for (int idx = 0; idx < RADIO_CLOUD_HISTORY_SIZE; idx++)
    {
        if (((history[idx] & APP_ID_MSK) >> 16) == app_id && (history[idx] & PACKET_ID_MSK) == id)
            return true;
    }

    return false;
}

void RadioCloud::addToHistory(uint32_t* history, uint8_t* history_index, uint16_t app_id, uint16_t id)
{
    history[*history_index] = ((app_id << 16) | id);
    *history_index = (*history_index + 1) % RADIO_CLOUD_HISTORY_SIZE;
}

/**
  * Protocol handler callback. This is called when the radio receives a packet marked as a datagram.
  *
  * This function process this packet, and queues it for user reception.
  */
void RadioCloud::packetReceived()
{
    RadioFrameBuffer* packet = radio.recv();
    DataPacket* temp = (DataPacket*)packet->payload;

    // ignore any packets that aren't part of our application
    // If appId == 0, we are in bridge mode -- accept all packets
    if (appId != 0 && temp->app_id != this->appId)
    {
        delete packet;
        return;
    }

    // if an ack, update our packet status
    if (temp->request_type & REQUEST_STATUS_ACK)
    {
        DataPacket* p = peakTxQueue(temp->id);

        // we don't expect a response from a node here.
        if (getBridgeMode())
        {
            p = removeFromTxQueue(temp->id);
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
        if (searchHistory(tx_history, temp->app_id, temp->id))
        {
            DataPacket* ack = new DataPacket();
            ack->app_id = temp->app_id;
            ack->id = temp->id;
            ack->request_type = REQUEST_STATUS_ACK;
            ack->status = 0;
            ack->len = 0;

            sendDataPacket(ack);
            delete ack;
        }

        // ignore previously received packets
        if (searchHistory(rx_history, temp->app_id, temp->id))
        {
            delete packet;
            return;
        }

        addToHistory(rx_history, &rx_history_index, temp->app_id, temp->id);
    }

    // we have received a response, remove any matching packets from the txQueue
    DataPacket* toDelete = removeFromQueue(&txQueue, temp->id);

    if (toDelete)
        delete toDelete;

    // add to our RX queue for app handling.
    DataPacket* p = new DataPacket();
    memcpy(p, packet->payload, packet->length - (MICROBIT_RADIO_HEADER_SIZE - 1));
    p->len = packet->length - (MICROBIT_RADIO_HEADER_SIZE - 1);

    p->no_response_count = 0;
    p->next = NULL;

    addToQueue(&rxQueue, p);

    delete packet;

    // interception event for Bridge.cpp
    MicroBitEvent(MICROBIT_RADIO_ID_CLOUD, p->id);

    // used for notifying underlying services that a packet has been received.
    // this occurs after the packet has been added to the rxQueue.
    if (temp->request_type & (REQUEST_TYPE_GET_REQUEST | REQUEST_TYPE_POST_REQUEST))
        rest.handlePacket(p->id);

    if (temp->request_type & REQUEST_TYPE_CLOUD_VARIABLE)
        variable.handlePacket(p->id);
}


DynamicType RadioCloud::recv(uint16_t id)
{
    DataPacket *t = removeFromRxQueue(id);

    if (t == NULL)
        return DynamicType();

    DynamicType dt;

    if (t->request_type == REQUEST_STATUS_ERROR)
        dt = DynamicType(7, (uint8_t*)"\01ERROR\0", DYNAMIC_TYPE_STATUS_ERROR);
    else
        dt = DynamicType(t->len - CLOUD_HEADER_SIZE, t->payload, 0);

    delete t;

    return dt;
}

DataPacket* RadioCloud::recvRaw(uint16_t id)
{
    return removeFromRxQueue(id);
}