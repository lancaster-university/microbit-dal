/**
  * Class definition for a MicroBitListener.
  *
  * MicroBitListener holds all the information related to a single event handler required
  * to match and fire event handlers to incoming events.
  */

#include "mbed.h"
#include "MicroBit.h"

/**
  * Constructor. 
  * Create a new Message Bus Listener.
  * @param id The ID of the component you want to listen to.
  * @param value The event ID you would like to listen to from that component
  * @param handler A function pointer to call when the event is detected.
  */
MicroBitListener::MicroBitListener(uint16_t id, uint16_t value, void (*handler)(MicroBitEvent))
{
	this->id = id;
	this->value = value;
	this->cb = handler;
	this->cb_arg = NULL;
    this->flags = 0;
	this->next = NULL;
}

/**
  * Constructor. 
  * Create a new parameterised Message Bus Listener.
  * @param id The ID of the component you want to listen to.
  * @param value The event ID you would like to listen to from that component.
  * @param handler A function pointer to call when the event is detected.
  * @param arg An additional argument to pass to the event handler function.
  */
MicroBitListener::MicroBitListener(uint16_t id, uint16_t value, void (*handler)(MicroBitEvent, void *), void* arg)
{
	this->id = id;
	this->value = value;
	this->cb_param = handler;
	this->cb_arg = arg;
    this->flags = MESSAGE_BUS_LISTENER_PARAMETERISED;
	this->next = NULL;
}

/**
 * Destructor. Ensures all resources used by this listener are freed.
 */
MicroBitListener::~MicroBitListener()
{
    if(this->flags & MESSAGE_BUS_LISTENER_METHOD)
        delete cb_method;
}
