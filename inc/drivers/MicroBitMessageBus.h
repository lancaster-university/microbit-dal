#ifndef MICROBIT_MESSAGE_BUS_H
#define MICROBIT_MESSAGE_BUS_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "MicroBitEvent.h"
#include "MicroBitListener.h"
#include "EventModel.h"

// Enumeration of core components.
/**
  * Class definition for the MicroBitMessageBus.
  *
  * The MicroBitMessageBus is the common mechanism to deliver asynchronous events on the
  * MicroBit platform. It serves a number of purposes:
  *
  * 1) It provides an eventing abstraction that is independent of the underlying substrate.
  * 2) It provides a mechanism to decouple user code from trusted system code
  *    i.e. the basis of a message passing nano kernel.
  * 3) It allows a common high level eventing abstraction across a range of hardware types.e.g. buttons, BLE...
  * 4) It provides a mechanims for extensibility - new devices added via I/O pins can have OO based
       drivers and communicate via the message bus with minima impact on user level languages.
  * 5) It allows for the possiblility of event / data aggregation, which in turn can save energy.
  * It has the following design principles:
  *
  * 1) Maintain a low RAM footprint where possible
  * 2) Make few assumptions about the underlying platform, but allow optimizations where possible.
  */
class MicroBitMessageBus : public EventModel, public MicroBitComponent
{
    public:

	/**
	  * Default constructor.
	  * Anticipating only one MessageBus per device, as filtering is handled within the class.
	  */
    MicroBitMessageBus();

	/**
	  * Queues the given event to be sent to all registered recipients.
	  *
	  * @param The event to send.
	  *
	  * n.b. THIS IS NOW WRAPPED BY THE MicroBitEvent CLASS FOR CONVENIENCE...
	  *
	  * Example:
      * @code
	  * MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_DOWN,ticks,false);
	  * evt.fire();
	  * //OR YOU CAN DO THIS...
	  * MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_DOWN);
      * @endcode
	  */
	virtual int send(MicroBitEvent evt);

	/**
      * Internal function, used to deliver the given event to all relevant recipients.
      * Normally, this is called once an event has been removed from the event queue.
      *
      * IT IS RECOMMENDED THAT ALL EXTERNAL CODE USE THE send() FUNCTIONS INSTEAD OF THIS FUNCTION,
      * or the constructors provided by MicroBitEvent.
	  *
	  * @param evt The event to send.
      * @param urgent The type of listeners to process (optional). If set to true, only listeners defined as urgent and non-blocking will be processed
      * otherwise, all other (standard) listeners will be processed.
      * @return 1 if all matching listeners were processed, 0 if further processing is required.
	  */
	int process(MicroBitEvent &evt, bool urgent = false);

    /**
      * Returns the microBitListener with the given position in our list.
      * @param n The position in the list to return.
      * @return the MicroBitListener at postion n in the list, or NULL if the position is invalid.
      */
    virtual MicroBitListener *elementAt(int n);

    /**
      * Destructor for MicroBitMessageBus, so that we deregister ourselves as an idleComponent
      */
    ~MicroBitMessageBus();

    /**
     * Add the given MicroBitListener to the list of event handlers, unconditionally.
     * @param listener The MicroBitListener to add.
     * @return MICROBIT_OK if the listener is valid, MICROBIT_INVALID_PARAMETER otherwise.
     */
    virtual int add(MicroBitListener *newListener);

    /**
     * Remove the given MicroBitListener from the list of event handlers.
     * @param listener The MicroBitListener to remove.
     * @return MICROBIT_OK if the listener is valid, MICROBIT_INVALID_PARAMETER otherwise.
     */
    virtual int remove(MicroBitListener *newListener);

	private:
    /**
     * Cleanup any MicroBitListeners marked for deletion from the list.
     * @return The number of listeners removed from the list.
     */
    int deleteMarkedListeners();

	MicroBitListener            *listeners;		    // Chain of active listeners.
    MicroBitEventQueueItem      *evt_queue_head;    // Head of queued events to be processed.
    MicroBitEventQueueItem      *evt_queue_tail;    // Tail of queued events to be processed.
    uint16_t                    nonce_val;          // The last nonce issued.
    uint16_t                    queueLength;        // The number of events currently waiting to be processed.

    void queueEvent(MicroBitEvent &evt);
    MicroBitEventQueueItem* dequeueEvent();

    virtual void idleTick();
    virtual int isIdleCallbackNeeded();
};

#endif
