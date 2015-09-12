/**
  * Class definition for a MicroBitEvent.
  *
  * The MicroBitEvent is the event object that represents an event that has occurred on the ubit.
  */

#include "MicroBit.h"

/**
  * Constructor. 
  * @param src ID of the MicroBit Component that generated the event e.g. MICROBIT_ID_BUTTON_A.
  * @param value Component specific code indicating the cause of the event.
  * @param fire whether the event should be fire immediately upon construction
  * 
  * Example:
  * @code 
  * MicrobitEvent evt(id,MICROBIT_BUTTON_EVT_CLICK,true); // auto fire
  * @endcode
  */
MicroBitEvent::MicroBitEvent(uint16_t source, uint16_t value, MicroBitEventLaunchMode mode)
{
    this->source = source;
    this->value = value;
    this->timestamp = ticks;
    
    if(mode != CREATE_ONLY)
        this->fire(mode);
}    

/**
  * Default constructor - initialises all values, and sets timestamp to the current time.
  */
MicroBitEvent::MicroBitEvent()
{
    this->source = 0;
    this->value = 0;
    this->timestamp = ticks;
}

/**
  * Fires the represented event onto the message bus.
  */
void MicroBitEvent::fire(MicroBitEventLaunchMode mode)
{
    if (mode == CREATE_AND_QUEUE)
        uBit.MessageBus.send(*this);

    else if (mode == CREATE_AND_FIRE)
        uBit.MessageBus.process(*this);
}

/**
  * Fires the represented event onto the message bus.
  */
void MicroBitEvent::fire()
{
    fire(MICROBIT_EVENT_DEFAULT_LAUNCH_MODE);
}


/**
  * Constructor. 
  * Create a new MicroBitEventQueueItem.
  * @param evt The event to be queued.
  */
MicroBitEventQueueItem::MicroBitEventQueueItem(MicroBitEvent evt)
{
    this->evt = evt;
	this->next = NULL;
}

