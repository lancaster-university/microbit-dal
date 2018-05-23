#include "RadioCloud.h"

RadioCloud::RadioCloud(Radio& r) : radio(r)
{
    this->txQueue = NULL;
    this->rxQueue = NULL;

    fiber_add_idle_component(this);
}

DataPacket* RadioCloud::removeFromQueue(DataPacket** queue, uint16_t id)
{
    DataPacket *p = *queue;
    DataPacket *previous = *queue;

    __disable_irq();
    while (p != NULL)
    {
        if (id == p->id)
        {
            if (p == *queue)
                *queue = p->next;
            else
                previous->next = p->next;

            return p;
        }

        previous = p;
        p = p->next;
    }
    __enable_irq();

    return NULL;
}

int RadioCloud::addToQueue(DataPacket** queue, DataPacket* packet)
{
    int queueDepth = 0;
    packet->next = NULL;

    __disable_irq();
    if (*queue == NULL)
    {
        *queue = packet;
    }
    else
    {
        DataPacket *p = *queue;

        while (p->next != NULL)
        {
            p = p->next;
            queueDepth++;
        }

        if (queueDepth >= REST_RADIO_MAXIMUM_TX_BUFFERS)
        {
            delete packet;
            return MICROBIT_NO_RESOURCES;
        }

        p->next = packet;
    }
    __enable_irq();

    return MICROBIT_OK;
}

int RadioCloud::addToTxQueue(DataPacket* p)
{
    return addToQueue(&txQueue, p);
}

DataPacket* RadioCloud::removeFromRxQueue(uint16_t id)
{
    return removeFromQueue(&rxQueue, id);
}

int RadioCloud::setAppId(uint16_t id)
{
    this->app_id = id;
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
    bool pop = false;

    DataPacket* p = txQueue;

    while(p != NULL)
    {
        // only transmit once every idle tick
        if (p->status & DATA_PACKET_WAITING_FOR_SEND && !transmitted)
        {
            log_string("SENDING!!!!!");
            transmitted = true;
            sendDataPacket(p);
            p->status = DATA_PACKET_WAITING_FOR_ACK;
            p->no_response_count = 0;
            p->retry_count = 0;
        }
        else if (p->status & DATA_PACKET_WAITING_FOR_ACK)
        {
             p->no_response_count++;

            if (p->no_response_count > REST_RADIO_NO_ACK_THRESHOLD && !transmitted)
            {
                p->retry_count++;

                if (p->retry_count > REST_RADIO_RETRY_THRESHOLD)
                    pop = true;

                sendDataPacket(p);
                p->no_response_count = 0;
                transmitted = true;
            }
        }
        else if (p->status & DATA_PACKET_ACK_RECEIVED)
        {
            p->no_response_count++;

            if (p->no_response_count > REST_RADIO_NO_RESPONSE_THRESHOLD)
                pop = true;
        }

        if (pop)
        {
            pop = false;
            DataPacket *t = removeFromQueue(&txQueue, p->id);

            uint8_t rt = t->request_type;
            t->request_type = REQUEST_TYPE_STATUS_ERROR;

            // expect client code to check for errors...
            addToQueue(&rxQueue, t);

            if (rt & (REQUEST_TYPE_GET_REQUEST | REQUEST_TYPE_PUT_REQUEST | REQUEST_TYPE_POST_REQUEST | REQUEST_TYPE_DELETE_REQUEST))
                rest.handleTimeout(p->id);

            if (rt & REQUEST_TYPE_CLOUD_VARIABLE)
                variable.handleTimeout(p->id);
        }

        p = p->next;
    }
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

    log_string("RX: ");
    log_num((int) this->appId);
    log_num((int) temp->app_id);

    // ignore any packets that aren't part of our application
    if (appId != 0 && temp->app_id != this->appId)
    {
        log_string("IGNORED");
        delete packet;
        return;
    }

    if (temp->request_type & REQUEST_TYPE_STATUS_ACK)
    {
        DataPacket* p = txQueue;

        while (p != NULL)
        {
            if (temp->id == p->id)
                break;

            p = p->next;
        }

        if (p)
        {
            p->status = DATA_PACKET_ACK_RECEIVED;
            p->no_response_count = 0;
            p->retry_count = 0;
        }

        delete packet;

        return;
    }

    log_string("AFTER");

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

    // used for notifying underlying services that a packet has been received.
    // this occurs after the packet has been added to the rxQueue.
    if (temp->request_type & (REQUEST_TYPE_GET_REQUEST | REQUEST_TYPE_PUT_REQUEST | REQUEST_TYPE_POST_REQUEST | REQUEST_TYPE_DELETE_REQUEST))
        rest.handlePacket(p);

    if (temp->request_type & REQUEST_TYPE_CLOUD_VARIABLE)
        variable.handlePacket(p);

    // send an ACK immediately
    DataPacket* ack = new DataPacket();
    p->app_id = p->app_id;
    p->id = p->id;
    p->request_type = REQUEST_TYPE_STATUS_ACK;
    p->status = 0;
    p->len = 0;

    sendDataPacket(ack);
    delete ack;
}


DynamicType RadioCloud::recv(uint16_t id)
{
    DataPacket *t = removeFromRxQueue(id);

    if (t == NULL)
        return DynamicType();

    log_string("VALID\r\n");
    log_num(t->len);
    log_num(t->len - CLOUD_HEADER_SIZE);

    DynamicType dt;

    if (t->request_type == REQUEST_TYPE_STATUS_ERROR)
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