#ifndef MICROBIT_LISTENER_H
#define MICROBIT_LISTENER_H

#include "mbed.h"
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
	
	MicroBitListener *next;

	/**
	  * Constructor. 
	  * Create a new Message Bus Listener.
	  * @param id The ID of the component you want to listen to.
	  * @param value The event ID you would like to listen to from that component
	  * @param handler A function pointer to call when the event is detected.
	  */
	MicroBitListener(uint16_t id, uint16_t value, void (*handler)(MicroBitEvent), uint16_t flags = MESSAGE_BUS_LISTENER_DEFAULT_FLAGS);
	
	/**
	  * Alternative constructor where we register a value to be passed to the
	  * callback. 
	  */
    MicroBitListener(uint16_t id, uint16_t value, void (*handler)(MicroBitEvent, void *), void* arg, uint16_t flags = MESSAGE_BUS_LISTENER_DEFAULT_FLAGS);

    /**
     * Constructor. 
     * Create a new Message Bus Listener, with a callback to a c++ member function.
     * @param id The ID of the component you want to listen to.
     * @param value The event ID you would like to listen to from that component.
     * @param object The C++ object on which to call the event handler.
     * @param object The method within the C++ object to call.
     */
    template <typename T> 
    MicroBitListener(uint16_t id, uint16_t value, T* object, void (T::*method)(MicroBitEvent), uint16_t flags = MESSAGE_BUS_LISTENER_DEFAULT_FLAGS);

    /**
      * Destructor. Ensures all resources used by this listener are freed.
      */
    ~MicroBitListener();

    /**
     * Queues and event up to be processed.
     * @param e The event to queue
     */
    void queue(MicroBitEvent e);
};

/**
  * Constructor. 
  * Create a new Message Bus Listener, with a callback to a c++ member function.
  * @param id The ID of the component you want to listen to.
  * @param value The event ID you would like to listen to from that component.
  * @param object The C++ object on which to call the event handler.
  * @param object The method within the C++ object to call.
  */

template <typename T>
MicroBitListener::MicroBitListener(uint16_t id, uint16_t value, T* object, void (T::*method)(MicroBitEvent), uint16_t flags)
{
	this->id = id;
	this->value = value;
    this->cb_method = new MemberFunctionCallback(object, method);
	this->cb_arg = NULL;
    this->flags = flags | MESSAGE_BUS_LISTENER_METHOD;
	this->next = NULL;
}

#endif


