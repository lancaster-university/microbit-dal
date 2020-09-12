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

#ifndef MICROBIT_LISTENER_H
#define MICROBIT_LISTENER_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitLock.h"
#include "MicroBitEvent.h"
#include "MemberFunctionCallback.h"

// MicroBitListener flags...
#define MESSAGE_BUS_LISTENER_PARAMETERISED          0x0001
#define MESSAGE_BUS_LISTENER_METHOD                 0x0002
#define MESSAGE_BUS_LISTENER_BUSY                   0x0004
#define MESSAGE_BUS_LISTENER_REENTRANT              0x0008
#define MESSAGE_BUS_LISTENER_QUEUE_IF_BUSY          0x0010
#define MESSAGE_BUS_LISTENER_DROP_IF_BUSY           0x0020
#define MESSAGE_BUS_LISTENER_NONBLOCKING            0x0040
#define MESSAGE_BUS_LISTENER_URGENT                 0x0080
#define MESSAGE_BUS_LISTENER_DELETING               0x8000

#define MESSAGE_BUS_LISTENER_IMMEDIATE              (MESSAGE_BUS_LISTENER_NONBLOCKING |  MESSAGE_BUS_LISTENER_URGENT)

/**
  *	This structure defines a MicroBitListener used to invoke functions, or member
  * functions if an instance of EventModel receives an event whose id and value
  * match this MicroBitListener's id and value.
  */
struct MicroBitListener
{
	uint16_t		id;				// The ID of the component that this listener is interested in.
	uint16_t 		value;			// Value this listener is interested in receiving.
    uint16_t        flags;          // Status and configuration options codes for this listener.

    union
    {
        void (*cb)(MicroBitEvent);
        void (*cb_param)(MicroBitEvent, void *);
        MemberFunctionCallback *cb_method;
    };

	void*			cb_arg;			// Optional argument to be passed to the caller.

	MicroBitEvent 	            evt;
	MicroBitEventQueueItem 	    *evt_queue;
    MicroBitLock                lock;
	MicroBitListener *next;

	/**
	  * Constructor.
	  *
	  * Create a new Message Bus Listener.
	  *
	  * @param id The ID of the component you want to listen to.
	  *
	  * @param value The event value you would like to listen to from that component
	  *
	  * @param handler A function pointer to call when the event is detected.
	  *
	  * @param flags User specified, implementation specific flags, that allow behaviour of this events listener
      * to be tuned.
	  */
	MicroBitListener(uint16_t id, uint16_t value, void (*handler)(MicroBitEvent), uint16_t flags = EVENT_LISTENER_DEFAULT_FLAGS);

	/**
	  * Constructor.
	  *
	  * Create a new Message Bus Listener, this constructor accepts an additional
	  * parameter "arg", which is passed to the handler.
	  *
	  * @param id The ID of the component you want to listen to.
	  *
	  * @param value The event value you would like to listen to from that component
	  *
	  * @param handler A function pointer to call when the event is detected.
	  *
	  * @param arg A pointer to some data that will be given to the handler.
	  *
	  * @param flags User specified, implementation specific flags, that allow behaviour of this events listener
      * to be tuned.
	  */
    MicroBitListener(uint16_t id, uint16_t value, void (*handler)(MicroBitEvent, void *), void* arg, uint16_t flags = EVENT_LISTENER_DEFAULT_FLAGS);


	/**
	  * Constructor.
	  *
	  * Create a new Message Bus Listener, with a callback to a C++ member function.
	  *
	  * @param id The ID of the component you want to listen to.
	  *
	  * @param value The event value you would like to listen to from that component
	  *
	  * @param object The C++ object on which to call the event handler.
	  *
      * @param method The method within the C++ object to call.
	  *
	  * @param flags User specified, implementation specific flags, that allow behaviour of this events listener
      * to be tuned.
	  */
    template <typename T>
    MicroBitListener(uint16_t id, uint16_t value, T* object, void (T::*method)(MicroBitEvent), uint16_t flags = EVENT_LISTENER_DEFAULT_FLAGS);

    /**
      * Destructor. Ensures all resources used by this listener are freed.
      */
    ~MicroBitListener();

    /**
      * Queues and event up to be processed.
	  *
      * @param e The event to queue
      */
    void queue(MicroBitEvent e);
};

/**
  * Constructor.
  *
  * Create a new Message Bus Listener, with a callback to a C++ member function.
  *
  * @param id The ID of the component you want to listen to.
  *
  * @param value The event value you would like to listen to from that component
  *
  * @param object The C++ object on which to call the event handler.
  *
  * @param method The method within the C++ object to call.
  *
  * @param flags User specified, implementation specific flags, that allow behaviour of this events listener
  * to be tuned.
  */
template <typename T>
MicroBitListener::MicroBitListener(uint16_t id, uint16_t value, T* object, void (T::*method)(MicroBitEvent), uint16_t flags)
{
	this->id = id;
	this->value = value;
    this->cb_method = new MemberFunctionCallback(object, method);
	this->cb_arg = NULL;
    this->flags = flags | MESSAGE_BUS_LISTENER_METHOD;
    this->evt_queue = NULL;
	this->next = NULL;
}

#endif
