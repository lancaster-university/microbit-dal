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
MicroBitEvent::MicroBitEvent(uint16_t source, uint16_t value, bool fire)
{
    this->source = source;
    this->value = value;
    this->timestamp = ticks;
    
    if(fire)
        this->fire();
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
void MicroBitEvent::fire()
{
    uBit.MessageBus.send(*this);
}
