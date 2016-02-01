#ifndef MICROBIT_RADIO_EVENT_H
#define MICROBIT_RADIO_EVENT_H

#include "mbed.h"
#include "MicroBitRadio.h"

/**
 * Provides a simple broadcast radio abstraction, built upon the raw nrf51822 RADIO module.
 *
 * This class provides the ability to extend the micro:bit's MessageBus to other micro:bits in the vicinity,
 * in a very similar way to the MicroBitEventService for BLE interfaces.
 * It is envisaged that this would provide the basis for children to experiment with building their own, simple,
 * custom asynchronous events and actions.
 *
 * NOTE: This API does not contain any form of encryption, authentication or authorisation. Its purpose is solely for use as a
 * teaching aid to demonstrate how simple communications operates, and to provide a sandpit through which learning can take place.
 * For serious applications, BLE should be considered a substantially more secure alternative.
 */

class MicroBitRadioEvent 
{
    bool       suppressForwarding;      // A private flag used to prevent event forwarding loops.

    public:

    /**
     * Constructor.
     */
    MicroBitRadioEvent();

    /**
     * Associates the given MessageBus events with the radio channel.
     * Once registered, all events matching the given registration sent to this micro:bit's 
     * MessageBus will be automatically retrasmitted on the radio.
     *
     * @param id The ID of the events to register.
     * @param value the VALUE of the event to register. use MICROBIT_EVT_ANY for all event values matching the given ID.
     *
     * @return MICROBIT_OK on success.
     */
    int listen(uint16_t id, uint16_t value);

    /**
     * Disassociates the given MessageBus events with the radio channel.
     *
     * @param id The ID of the events to deregister.
     * @param value the VALUE of the event to deregister. use MICROBIT_EVT_ANY for all event values matching the given ID.
     *
     * @return MICROBIT_OK on success.
     */
    int ignore(uint16_t id, uint16_t value);

    /**
     * Protocol handler callback. This is called when the radio receives a packet marked as using the event protocol.
     * This function process this packet, and fires the event contained inside onto the local MessageBus. 
     */
    void packetReceived();

    /**
     * Event handler callback. This is called whenever an event is received matching one of those registered through
     * the registerEvent() method described above. Upon receiving such an event, it is wrapped into
     * a radio packet and transmitted to any othe rmicro:bits in the same group.
     */
    void eventReceived(MicroBitEvent e);
};

#endif
