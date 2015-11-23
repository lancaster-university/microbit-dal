#include "MicroBit.h"

/**
 * Provides a simple broadcast radio abstraction, built upon the raw nrf51822 RADIO module.
 *
 * This class provides the ability to extend the micro:bit's MessageBus to other micro:bits in the vicinity,
 * in a very similar way to the MicroBitEventService for BLE interfaces.
 * It is envisaged that this would provide the basis for children to experiment with building their own, simple,
 * custom asynchronous events.
 *
 * NOTE: This API does not contain any form of encryption, authentication or authorisation. Its purpose is solely for use as a
 * teaching aid to demonstrate how simple communications operates, and to provide a sandpit through which learning can take place.
 * For serious applications, BLE should be considered a substantially more secure alternative.
 */

/**
  * Constructor.
  * @param radio The underlying radio module that this service will use.
  */
MicroBitRadioEvent::MicroBitRadioEvent(MicroBitRadio &r) : radio(r)
{
    this->suppressForwarding = false;
}

/**
 * Associates the given MessageBus events with the radio channel.
 * Once registered, all events matching the given registration sent to this micro:bit's 
 * default MessageBus will be automaticlaly retrasmitted on the radio.
 *
 * @param id The ID of the events to register.
 * @param value the VALUE of the event to register. use MICROBIT_EVT_ANY for all event values matching the given id.
 *
 * @return MICROBIT_OK on success, or MICROBIT_NO_RESOURCES if no defult MessageBus is available.
 */
int MicroBitRadioEvent::listen(uint16_t id, uint16_t value)
{
    if (MicroBitMessageBus::defaultMessageBus)
        return listen(id, value, *MicroBitMessageBus::defaultMessageBus);

    return MICROBIT_NO_RESOURCES;
}

/**
 * Associates the given MessageBus events with the radio channel.
 * Once registered, all events matching the given registration sent to the given 
 * MessageBus will be automaticlaly retrasmitted on the radio.
 *
 * @param id The ID of the events to register.
 * @param value the VALUE of the event to register. use MICROBIT_EVT_ANY for all event values matching the given id.
 *
 * @return MICROBIT_OK on success.
 */
int MicroBitRadioEvent::listen(uint16_t id, uint16_t value, MicroBitMessageBus &messageBus)
{
    return messageBus.listen(id, value, this, &MicroBitRadioEvent::eventReceived, MESSAGE_BUS_LISTENER_IMMEDIATE);
}

/**
 * Disassociates the given MessageBus events with the radio channel.
 *
 * @param id The ID of the events to deregister.
 * @param value the VALUE of the event to deregister. use MICROBIT_EVT_ANY for all event values matching the given id.
 *
 * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if no default MessageBus is available.
 */
int MicroBitRadioEvent::ignore(uint16_t id, uint16_t value)
{
    if (MicroBitMessageBus::defaultMessageBus)
        return ignore(id, value, *MicroBitMessageBus::defaultMessageBus);

    return MICROBIT_INVALID_PARAMETER;
}

/**
 * Disassociates the given MessageBus events with the radio channel.
 *
 * @param id The ID of the events to deregister.
 * @param value the VALUE of the event to deregister. use MICROBIT_EVT_ANY for all event values matching the given id.
 * @param The message bus to deregister on.
 *
 * @return MICROBIT_OK on success.
 */
int MicroBitRadioEvent::ignore(uint16_t id, uint16_t value, MicroBitMessageBus &messageBus)
{
    return messageBus.ignore(id, value, this, &MicroBitRadioEvent::eventReceived);
}


/**
 * Protocol handler callback. This is called when the radio receives a packet marked as an event
 * This function process this packet, and fires the event contained inside onto the local MessageBus. 
 */
void MicroBitRadioEvent::packetReceived()
{
    FrameBuffer *p = radio.recv();
    MicroBitEvent *e = (MicroBitEvent *) p->payload;

    suppressForwarding = true;
    e->fire();
    suppressForwarding = false;

    delete p;
}

/**
 * Event handler callback. This is called whenever an event is received matching one of those registered through
 * the registerEvent() method described above. Upon receiving such an event, it is wrapped into
 * a radio packet and transmitted to any othe rmicro:bits in the same group.
 */
void MicroBitRadioEvent::eventReceived(MicroBitEvent e)
{
    if(suppressForwarding)
        return;

    FrameBuffer buf;

    buf.length = sizeof(MicroBitEvent) + MICROBIT_RADIO_HEADER_SIZE - 1;
    buf.version = 1;
    buf.group = 0;
    buf.protocol = MICROBIT_RADIO_PROTOCOL_EVENTBUS;
    memcpy(buf.payload, (const uint8_t *)&e, sizeof(MicroBitEvent));

    radio.send(&buf);
}

