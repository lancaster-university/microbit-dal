#ifndef MICROBIT_MESSAGE_BUS_H
#define MICROBIT_MESSAGE_BUS_H

#include "mbed.h"
#include "MicroBitComponent.h"
#include "MicroBitEvent.h"
#include "MicroBitListener.h"

// Enumeration of core components.
#define MICROBIT_CONTROL_BUS_ID         0
#define MICROBIT_ID_ANY					0
#define MICROBIT_EVT_ANY				0

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
class MicroBitMessageBus : public MicroBitComponent
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
	void send(MicroBitEvent evt);

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
	  * Register a listener function.
	  *
	  * @param id The source of messages to listen for. Events sent from any other IDs will be filtered.
	  * Use MICROBIT_ID_ANY to receive events from all components.
	  *
	  * @param value The value of messages to listen for. Events with any other values will be filtered.
	  * Use MICROBIT_EVT_ANY to receive events of any value.
	  *
	  * @param handler The function to call when an event is received.
      *
      * @return MICROBIT_OK on success MICROBIT_INVALID_PARAMETER
	  *
      * Example:
      * @code
      * void onButtonBClick()
      * {
      * 	//do something
      * }
      * uBit.MessageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, onButtonBClick); // call function when ever a click event is detected.
      * @endcode
	  */
	int listen(int id, int value, void (*handler)(MicroBitEvent), uint16_t flags = MESSAGE_BUS_LISTENER_DEFAULT_FLAGS);

	/**
	  * Register a listener function.
	  *
	  * @param id The source of messages to listen for. Events sent from any other IDs will be filtered.
	  * Use MICROBIT_ID_ANY to receive events from all components.
	  *
	  * @param value The value of messages to listen for. Events with any other values will be filtered.
	  * Use MICROBIT_EVT_ANY to receive events of any value.
	  *
	  * @param hander The function to call when an event is received.
      *
      * @return MICROBIT_OK on success MICROBIT_INVALID_PARAMETER
	  *
      * Example:
      * @code
      * void onButtonBClick(void *arg)
      * {
      * 	//do something
      * }
      * uBit.MessageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, onButtonBClick); // call function when ever a click event is detected.
      * @endcode
	  */
	int listen(int id, int value, void (*handler)(MicroBitEvent, void*), void* arg, uint16_t flags = MESSAGE_BUS_LISTENER_DEFAULT_FLAGS);

	/**
	  * Register a listener function.
	  *
	  * @param id The source of messages to listen for. Events sent from any other IDs will be filtered.
	  * Use MICROBIT_ID_ANY to receive events from all components.
	  *
	  * @param value The value of messages to listen for. Events with any other values will be filtered.
	  * Use MICROBIT_EVT_ANY to receive events of any value.
	  *
	  * @param hander The function to call when an event is received.
      *
      * @return MICROBIT_OK on success MICROBIT_INVALID_PARAMETER
	  *
      * Example:
      * @code
      * void SomeClass::onButtonBClick()
      * {
      * 	//do something
      * }
      *
      * SomeClass s = new SomeClass();
      * uBit.MessageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, s, &SomeClass::onButtonBClick);
      * @endcode
	  */
    template <typename T>
	int listen(uint16_t id, uint16_t value, T* object, void (T::*handler)(MicroBitEvent), uint16_t flags = MESSAGE_BUS_LISTENER_DEFAULT_FLAGS);


	/**
	  * Unregister a listener function.
      * Listners are identified by the Event ID, Event VALUE and handler registered using listen().
	  *
	  * @param id The Event ID used to register the listener.
	  * @param value The Event VALUE used to register the listener.
	  * @param handler The function used to register the listener.
      *
      * @return MICROBIT_OK on success MICROBIT_INVALID_PARAMETER
      *
	  *
      * Example:
      * @code
      * void onButtonBClick()
      * {
      * 	//do something
      * }
      *
      * uBit.MessageBus.ignore(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, onButtonBClick);
      * @endcode
	  */
	int ignore(int id, int value, void (*handler)(MicroBitEvent));

	/**
	  * Unregister a listener function.
      * Listners are identified by the Event ID, Event VALUE and handler registered using listen().
	  *
	  * @param id The Event ID used to register the listener.
	  * @param value The Event VALUE used to register the listener.
	  * @param handler The function used to register the listener.
      *
      * @return MICROBIT_OK on success MICROBIT_INVALID_PARAMETER
	  *
      * Example:
      * @code
      * void onButtonBClick(void *arg)
      * {
      * 	//do something
      * }
      *
      * uBit.MessageBus.ignore(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, onButtonBClick);
      * @endcode
	  */
	int ignore(int id, int value, void (*handler)(MicroBitEvent, void*));

	/**
	  * Unregister a listener function.
      * Listners are identified by the Event ID, Event VALUE and handler registered using listen().
	  *
	  * @param id The Event ID used to register the listener.
	  * @param value The Event VALUE used to register the listener.
	  * @param handler The function used to register the listener.
      *
      * @return MICROBIT_OK on success MICROBIT_INVALID_PARAMETER
	  *
      * Example:
      * @code
      *
      * void SomeClass::onButtonBClick()
      * {
      * 	//do something
      * }
      *
      * SomeClass s = new SomeClass();
      * uBit.MessageBus.ignore(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, s, &SomeClass::onButtonBClick);
      * @endcode
	  */
    template <typename T>
	int ignore(uint16_t id, uint16_t value, T* object, void (T::*handler)(MicroBitEvent));

    /**
      * Returns the microBitListener with the given position in our list.
      * @param n The position in the list to return.
      * @return the MicroBitListener at postion n in the list, or NULL if the position is invalid.
      */
    MicroBitListener *elementAt(int n);

    /**
      * Destructor for MicroBitMessageBus, so that we deregister ourselves as an idleComponent
      */
    ~MicroBitMessageBus();

	private:

    /**
     * Add the given MicroBitListener to the list of event handlers, unconditionally.
     * @param listener The MicroBitListener to add.
     * @return MICROBIT_OK if the listener is valid, MICROBIT_INVALID_PARAMETER otherwise.
     */
    int add(MicroBitListener *newListener);

    /**
     * Remove the given MicroBitListener from the list of event handlers.
     * @param listener The MicroBitListener to remove.
     * @return MICROBIT_OK if the listener is valid, MICROBIT_INVALID_PARAMETER otherwise.
     */
    int remove(MicroBitListener *newListener);

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

/**
  * A registration function to allow C++ member funcitons (methods) to be registered as an event
  * listener.
  *
  * @param id The source of messages to listen for. Events sent from any other IDs will be filtered.
  * Use MICROBIT_ID_ANY to receive events from all components.
  *
  * @param value The value of messages to listen for. Events with any other values will be filtered.
  * Use MICROBIT_EVT_ANY to receive events of any value.
  *
  * @param object The object on which the method should be invoked.
  * @param handler The method to call when an event is received.
  *
  * @return MICROBIT_OK on success MICROBIT_INVALID_PARAMETER
  */
template <typename T>
int MicroBitMessageBus::listen(uint16_t id, uint16_t value, T* object, void (T::*handler)(MicroBitEvent), uint16_t flags)
{
	if (object == NULL || handler == NULL)
		return MICROBIT_INVALID_PARAMETER;

	MicroBitListener *newListener = new MicroBitListener(id, value, object, handler, flags);

    if(add(newListener) == MICROBIT_OK)
        return MICROBIT_OK;

    delete newListener;
    return MICROBIT_NO_RESOURCES;
}

/**
 * Unregister a listener function.
 * Listners are identified by the Event ID, Event VALUE and handler registered using listen().
 *
 * @param id The Event ID used to register the listener.
 * @param value The Event VALUE used to register the listener.
 * @param handler The function used to register the listener.
 *
 * @return MICROBIT_OK on success MICROBIT_INVALID_PARAMETER
 *
 * Example:
 * @code
 * void onButtonBClick(void *arg)
 * {
 * 	//do something
 * }
 *
 * uBit.MessageBus.ignore(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, onButtonBClick);
 * @endcode
 */
template <typename T>
int MicroBitMessageBus::ignore(uint16_t id, uint16_t value, T* object, void (T::*handler)(MicroBitEvent))
{
	if (handler == NULL)
		return MICROBIT_INVALID_PARAMETER;

	MicroBitListener listener(id, value, object, handler);
    remove(&listener);

    return MICROBIT_OK;
}


#endif
