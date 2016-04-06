/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef MICROBIT_RADIO_EVENT_H
#define MICROBIT_RADIO_EVENT_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitRadio.h"
#include "EventModel.h"

/**
 * Provides a simple broadcast radio abstraction, built upon the raw nrf51822 RADIO module.
 *
 * This class provides the ability to extend the micro:bit's default EventModel to other micro:bits in the vicinity,
 * in a very similar way to the MicroBitEventService for BLE interfaces.
 *
 * It is envisaged that this would provide the basis for children to experiment with building their own, simple,
 * custom asynchronous events and actions.
 *
 * @note This API does not contain any form of encryption, authentication or authorisation. Its purpose is solely for use as a
 * teaching aid to demonstrate how simple communications operates, and to provide a sandpit through which learning can take place.
 * For serious applications, BLE should be considered a substantially more secure alternative.
 */
class MicroBitRadioEvent
{
    bool            suppressForwarding;     // A private flag used to prevent event forwarding loops.
    MicroBitRadio   &radio;                 // A reference to the underlying radio module to use.

    public:

    /**
      * Constructor.
      *
      * Creates an instance of MicroBitRadioEvent which offers the ability to extend
      * the micro:bit's default EventModel to other micro:bits in the vicinity.
      *
      * @param r The underlying radio module used to send and receive data.
      */
    MicroBitRadioEvent(MicroBitRadio &r);

    /**
      * Associates the given event with the radio channel.
      *
      * Once registered, all events matching the given registration sent to this micro:bit's
      * default EventModel will be automatically retransmitted on the radio.
      *
      * @param id The id of the event to register.
      *
      * @param value the value of the event to register.
      *
      * @return MICROBIT_OK on success, or MICROBIT_NO_RESOURCES if no default EventModel is available.
      *
      * @note The wildcards MICROBIT_ID_ANY and MICROBIT_EVT_ANY can also be in place of the
      *       id and value fields.
      */
    int listen(uint16_t id, uint16_t value);

    /**
      * Associates the given event with the radio channel.
      *
      * Once registered, all events matching the given registration sent to the given
      * EventModel will be automatically retransmitted on the radio.
      *
      * @param id The id of the events to register.
      *
      * @param value the value of the event to register.
      *
      * @param eventBus The EventModel to listen for events on.
      *
      * @return MICROBIT_OK on success.
      *
      * @note The wildcards MICROBIT_ID_ANY and MICROBIT_EVT_ANY can also be in place of the
      *       id and value fields.
      */
    int listen(uint16_t id, uint16_t value, EventModel &eventBus);

    /**
      * Disassociates the given event with the radio channel.
      *
      * @param id The id of the events to deregister.
      *
      * @param value The value of the event to deregister.
      *
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER if the default message bus does not exist.
      *
      * @note MICROBIT_EVT_ANY can be used to deregister all event values matching the given id.
      */
    int ignore(uint16_t id, uint16_t value);

    /**
      * Disassociates the given events with the radio channel.
      *
      * @param id The id of the events to deregister.
      *
      * @param value The value of the event to deregister.
      *
      * @param eventBus The EventModel to deregister on.
      *
      * @return MICROBIT_OK on success.
      *
      * @note MICROBIT_EVT_ANY can be used to deregister all event values matching the given id.
      */
    int ignore(uint16_t id, uint16_t value, EventModel &eventBus);

    /**
      * Protocol handler callback. This is called when the radio receives a packet marked as using the event protocol.
      *
      * This function process this packet, and fires the event contained inside onto the default EventModel.
      */
    void packetReceived();

    /**
      * Event handler callback. This is called whenever an event is received matching one of those registered through
      * the registerEvent() method described above. Upon receiving such an event, it is wrapped into
      * a radio packet and transmitted to any other micro:bits in the same group.
      */
    void eventReceived(MicroBitEvent e);
};

#endif
