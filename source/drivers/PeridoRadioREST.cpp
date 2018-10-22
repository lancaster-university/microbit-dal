#include "MicroBitConfig.h"

// #if MICROBIT_RADIO_VERSION == MICROBIT_RADIO_MODIFIED

#include "MicroBitPeridoRadio.h"
#include "PeridoRadioREST.h"
#include "MicroBitFiber.h"

PeridoRadioREST::PeridoRadioREST(PeridoRadioCloud& r) : cloud(r)
{
}

DynamicType PeridoRadioREST::getRequest(ManagedString url)
{
    // + 2 for null terminator and type byte
    uint8_t bufLen = url.length() + 2;
    uint8_t* urlBuf = (uint8_t *)malloc(bufLen);
    urlBuf[0] |= SUBTYPE_STRING;

    memcpy(urlBuf + 1,url.toCharArray(), bufLen - 1);

    int ret = cloud.send(REQUEST_TYPE_GET_REQUEST, urlBuf, bufLen);
    free(urlBuf);

    fiber_wake_on_event(RADIO_REST_ID, ret);
    schedule();
    return recv(ret);
}

// returns the message bus value to use
uint16_t PeridoRadioREST::getRequestAsync(ManagedString url)
{
    uint8_t bufLen = url.length() + 2; // + 2 for null terminator and type byte
    uint8_t* urlBuf = (uint8_t *)malloc(bufLen);
    urlBuf[0] |= SUBTYPE_STRING;

    memcpy(urlBuf + 1, url.toCharArray(), bufLen - 1);

    int ret = cloud.send(REQUEST_TYPE_GET_REQUEST, urlBuf, bufLen);
    free(urlBuf);

    return ret;
}

DynamicType PeridoRadioREST::postRequest(ManagedString url, DynamicType& parameters)
{
    uint16_t strLen = url.length() + 1; // for null terminator

    uint16_t bufLen = strLen + 1 + parameters.length(); // + 1 for subtype byte
    uint8_t* urlBuf = (uint8_t *)malloc(bufLen);
    urlBuf[0] = SUBTYPE_STRING;

    memcpy(urlBuf + 1, url.toCharArray(), strLen);
    memcpy(urlBuf + 1 + strLen, parameters.getBytes(), parameters.length());

    int ret = cloud.send(REQUEST_TYPE_POST_REQUEST, urlBuf, bufLen);
    free(urlBuf);

    fiber_wake_on_event(RADIO_REST_ID, ret);
    schedule();

    return recv(ret);
}

uint16_t PeridoRadioREST::postRequestAsync(ManagedString url, DynamicType& parameters)
{
    uint16_t strLen = url.length() + 1; // for null terminator

    uint16_t bufLen = strLen + 1 + parameters.length(); // + 1 for subtype byte
    uint8_t* urlBuf = (uint8_t *)malloc(bufLen);
    urlBuf[0] |= SUBTYPE_STRING;

    memcpy(urlBuf + 1, url.toCharArray(), url.length() + 1);
    memcpy(urlBuf + 1 + strLen, parameters.getBytes(), parameters.length());

    int ret = cloud.send(REQUEST_TYPE_POST_REQUEST, urlBuf, bufLen);
    free(urlBuf);

    return ret;
}

void PeridoRadioREST::handleTimeout(uint16_t id)
{
    // wake any waiting fibers.
    MicroBitEvent(RADIO_REST_ID, id);
}

/**
  * Protocol handler callback. This is called when the radio receives a packet marked as a datagram.
  *
  * This function process this packet, and queues it for user reception.
  */
void PeridoRadioREST::handlePacket(uint16_t id)
{
    // wake any waiting fibers, to recv the packet.
    // TODO: who removes the packet if nothing reads it?!
    MicroBitEvent(RADIO_REST_ID, id);
}

DynamicType PeridoRadioREST::recv(uint16_t id)
{
    // probably also include a service type to differentiate packets.
    return cloud.recv(id);
}

CloudDataItem* PeridoRadioREST::recvRaw(uint16_t id)
{
    // probably also include a service type to differentiate packets.
    return cloud.recvRaw(id);
}

// #endif