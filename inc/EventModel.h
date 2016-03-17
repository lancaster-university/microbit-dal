#ifndef EVENT_MODEL_H
#define EVENT_MODEL_H

#include "mbed.h"
#include "MicroBitComponent.h"
#include "MicroBitEvent.h"
#include "MicroBitListener.h"
#include "ErrorNo.h"

/**
  * Class definition for the micro:bit EventModel.
  *
  * It is common to need to send events from one part of a program (or system) to another.
  * The way that these events are stored and delivered is known as an Event Model...
  *
  * The micro:bit can be programmed in a number of languages, and it not be good to
  * constrain those languages to any particular event model (e.g. they may have their own already).
  *
  * This class defines the functionality an event model needs to have to be able to interact
  * with events generated and/or used by the micro:bit runtime. Programmer may choose to implement
  * such funcitonality to integrate their own event models.
  *
  * This is an example of a key principle in computing - ABSTRACTION. This is now part of the
  * UK's Computing curriculum in schools... so ask your teacher about it. :-)
  *
  * An EventModel implementation is provided in the MicroBitMessageBus class.
  */
class EventModel
{
    public:

    static EventModel *defaultEventBus;
	/**
	  * Queues the given event to be sent to all registered recipients.
      * The method of delivery will vary depending on the
	  *
	  * @param The event to send.
      * @return MICROBIT_OK on success, or any valid error code defined in "ErrNo.h". The default implementation
      * simply returns MICROBIT_NOT_SUPPORTED.
	  */
	virtual int send(MicroBitEvent evt)
    {
        (void) evt;
        return MICROBIT_NOT_SUPPORTED;
    }

    /**
     * Add the given MicroBitListener to the list of event handlers, unconditionally.
     * @param listener The MicroBitListener to validate.
     * @return 1 if the listener is valid, 0 otherwise.
     */
    virtual int add(MicroBitListener *listener)
    {
        (void) listener;
        return MICROBIT_NOT_SUPPORTED;
    }

    /**
     * Remove the given MicroBitListener from the list of event handlers.
     * @param listener The MicroBitListener to remove.
     * @return MICROBIT_OK if the listener is valid, MICROBIT_INVALID_PARAMETER otherwise.
     */
    virtual int remove(MicroBitListener *listener)
    {
        (void) listener;
        return MICROBIT_NOT_SUPPORTED;
    }

    /**
      * Returns the microBitListener with the given position in our list.
      * @param n The position in the list to return.
      * @return the MicroBitListener at postion n in the list, or NULL if the position is invalid.
      */
    MicroBitListener *elementAt(int n)
    {
        (void) n;
        return NULL;
    }

	/**
	  * Define the default event model to use for events raised and consumed by the microbit-dal runtime.
      * The defult event model may be changed at any time.
      *
	  * @param model The new evnet model to use as the default service.
      *
      * @return MICROBIT_OK on success.
	  *
      * Example:
      * @code
      * MicroBitMessageBus b();
      * EventModel:setDefaultEventModel(b);
      * @endcode
	  */
	static int setDefaultEventModel(EventModel &model)
    {
        EventModel::defaultEventBus = &model;
        return MICROBIT_OK;
    }

	/**
	  * Register a listener function.
      *
      * An EventModel implementing this interface may optionally choose to override this method,
      * if that EventModel supports asynchronous callbacks to user code, but there is no
      * requirement to do so.
      *
	  * @param id The source of messages to listen for. Events sent from any other IDs will be filtered.
	  * Use MICROBIT_ID_ANY to receive events from all components.
	  *
	  * @param value The value of messages to listen for. Events with any other values will be filtered.
	  * Use MICROBIT_EVT_ANY to receive events of any value.
	  *
	  * @param handler The function to call when an event is received.
      *
      * @param flags user specified, implementaiton specific flags to allow behaviour of this event listeners
      * to be tuned.
      *
      * @return MICROBIT_OK on success, or any valid error code defined in "ErrNo.h". The default implementation
      * simply returns MICROBIT_NOT_SUPPORTED.
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
	int listen(int id, int value, void (*handler)(MicroBitEvent), uint16_t flags = EVENT_LISTENER_DEFAULT_FLAGS)
    {
        if (handler == NULL)
            return MICROBIT_INVALID_PARAMETER;

        MicroBitListener *newListener = new MicroBitListener(id, value, handler, flags);

        if(add(newListener) == MICROBIT_OK)
            return MICROBIT_OK;

        delete newListener;

        return MICROBIT_NOT_SUPPORTED;
    }

    int listen(int id, int value, void (*handler)(MicroBitEvent, void*), void* arg, uint16_t flags = EVENT_LISTENER_DEFAULT_FLAGS)
    {
        if (handler == NULL)
            return MICROBIT_INVALID_PARAMETER;

        MicroBitListener *newListener = new MicroBitListener(id, value, handler, arg, flags);

        if(add(newListener) == MICROBIT_OK)
            return MICROBIT_OK;

        delete newListener;

        return MICROBIT_NOT_SUPPORTED;
    }

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
	int listen(uint16_t id, uint16_t value, T* object, void (T::*handler)(MicroBitEvent), uint16_t flags = EVENT_LISTENER_DEFAULT_FLAGS);


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
	int ignore(int id, int value, void (*handler)(MicroBitEvent))
    {
        if (handler == NULL)
            return MICROBIT_INVALID_PARAMETER;

        MicroBitListener listener(id, value, handler);
        remove(&listener);

        return MICROBIT_OK;
    }

	/**
	  * Unregister a listener function.
      * Listeners are identified by the Event ID, Event VALUE and handler registered using listen().
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
	int ignore(int id, int value, void (*handler)(MicroBitEvent, void*))
    {
        if (handler == NULL)
            return MICROBIT_INVALID_PARAMETER;

        MicroBitListener listener(id, value, handler, NULL);
        remove(&listener);

        return MICROBIT_OK;
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

};

/**
  * A registration function to allow C++ member functions (methods) to be registered as an event
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
int EventModel::listen(uint16_t id, uint16_t value, T* object, void (T::*handler)(MicroBitEvent), uint16_t flags)
{
	if (object == NULL || handler == NULL)
		return MICROBIT_INVALID_PARAMETER;

	MicroBitListener *newListener = new MicroBitListener(id, value, object, handler, flags);

    if(add(newListener) == MICROBIT_OK)
        return MICROBIT_OK;

    delete newListener;
    return MICROBIT_NOT_SUPPORTED;
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
int EventModel::ignore(uint16_t id, uint16_t value, T* object, void (T::*handler)(MicroBitEvent))
{
	if (handler == NULL)
		return MICROBIT_INVALID_PARAMETER;

	MicroBitListener listener(id, value, object, handler);
    remove(&listener);

    return MICROBIT_OK;
}

#endif
