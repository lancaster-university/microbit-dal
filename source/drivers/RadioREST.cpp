#include "MicroBitConfig.h"

// #if MICROBIT_RADIO_VERSION == MICROBIT_RADIO_MODIFIED

#include "Radio.h"
#include "RadioREST.h"
#include "MicroBitFiber.h"

RadioREST::RadioREST(RadioCloud& r) : cloud(r)
{
}

DataPacket* RadioREST::composePacket(uint8_t type, uint8_t* payload, uint8_t payload_len, uint16_t packet_id)
{
    uint32_t id = (packet_id != 0) ? packet_id : microbit_random(65535);

    DataPacket* p = new DataPacket();
    p->app_id = cloud.appId;
    p->id = id;
    p->request_type = type;
    p->status = DATA_PACKET_WAITING_FOR_SEND;

    uint32_t len = min(MAX_PAYLOAD_SIZE, payload_len);

    p->len = len;

    // we don't always have any payload, like for ACKs.
    if (len > 0)
        memcpy(p->payload, payload, len);

    return p;
}

DynamicType RadioREST::getRequest(ManagedString url)
{
    // + 2 for null terminator and type byte
    uint8_t bufLen = url.length() + 2;
    uint8_t* urlBuf = (uint8_t *)malloc(bufLen);
    urlBuf[0] |= SUBTYPE_STRING;

    memcpy(urlBuf + 1,url.toCharArray(), bufLen - 1);

    DataPacket *p = composePacket(REQUEST_TYPE_GET_REQUEST, urlBuf, bufLen);

    free(urlBuf);
    uint32_t id = p->id;

    cloud.addToTxQueue(p);

    fiber_wake_on_event(RADIO_REST_ID, id);
    schedule();
    return recv(id);
}

// returns the message bus value to use
uint16_t RadioREST::getRequestAsync(ManagedString url)
{
    uint8_t bufLen = url.length() + 2; // + 2 for null terminator and type byte
    uint8_t* urlBuf = (uint8_t *)malloc(bufLen);
    urlBuf[0] |= SUBTYPE_STRING;

    memcpy(urlBuf + 1,url.toCharArray(), bufLen - 1);

    DataPacket *p = composePacket(REQUEST_TYPE_GET_REQUEST, urlBuf, bufLen);

    free(urlBuf);
    uint32_t id = p->id;

    cloud.addToTxQueue(p);
    return id;
}

DynamicType RadioREST::postRequest(ManagedString url, DynamicType& parameters)
{
    uint16_t strLen = url.length() + 1; // for null terminator

    uint16_t bufLen = strLen + 1 + parameters.length(); // + 1 for subtype byte
    uint8_t* urlBuf = (uint8_t *)malloc(bufLen);
    urlBuf[0] = SUBTYPE_STRING;

    memcpy(urlBuf + 1,url.toCharArray(), strLen);
    memcpy(urlBuf + 1 + strLen, parameters.getBytes(), parameters.length());

    DataPacket *p = composePacket(REQUEST_TYPE_POST_REQUEST, urlBuf, bufLen);
    free(urlBuf);
    uint32_t id = p->id;

    cloud.addToTxQueue(p);

    fiber_wake_on_event(RADIO_REST_ID, id);
    schedule();
    return recv(id);
}

uint16_t RadioREST::postRequestAsync(ManagedString url, DynamicType& parameters)
{
    uint16_t strLen = url.length() + 1; // for null terminator

    uint16_t bufLen = strLen + 1 + parameters.length(); // + 1 for subtype byte
    uint8_t* urlBuf = (uint8_t *)malloc(bufLen);
    urlBuf[0] |= SUBTYPE_STRING;

    memcpy(urlBuf + 1,url.toCharArray(), url.length() + 1);
    memcpy(urlBuf + 1 + strLen, parameters.getBytes(), parameters.length());

    DataPacket *p = composePacket(REQUEST_TYPE_POST_REQUEST, urlBuf, bufLen);
    free(urlBuf);
    uint32_t id = p->id;

    cloud.addToTxQueue(p);

    return id;
}

void RadioREST::handleTimeout(uint16_t id)
{
    // wake any waiting fibers.
    MicroBitEvent(RADIO_REST_ID, id);
}

/**
  * Protocol handler callback. This is called when the radio receives a packet marked as a datagram.
  *
  * This function process this packet, and queues it for user reception.
  */
void RadioREST::handlePacket(uint16_t id)
{
    // wake any waiting fibers, to recv the packet.
    // TODO: who removes the packet if nothing reads it?!
    MicroBitEvent(RADIO_REST_ID, id);
}

DynamicType RadioREST::recv(uint16_t id)
{
    // probably also include a service type to differentiate packets.
    return cloud.recv(id);
}

DataPacket* RadioREST::recvRaw(uint16_t id)
{
    // probably also include a service type to differentiate packets.
    return cloud.recvRaw(id);
}

// #endif