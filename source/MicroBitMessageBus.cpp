/**
  * Class definition for a MicroBitMessageBus.
  *
  * The MicroBitMessageBus handles all messages passed between components.
  */

#include "MicroBit.h"

/**
  * Constructor. 
  * Create a new MicroBitEventQueueItem.
  * @param evt The event that is to be queued.
  */
MicroBitEventQueueItem::MicroBitEventQueueItem(MicroBitEvent &evt)
{
    this->evt = evt;
	this->next = NULL;
}
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
	this->cb = (void*) handler;
	this->cb_arg = NULL;
	this->next = NULL;
}

/**
  * A low-level, internal version of the constructor, where the handler's type is determined
  * by the value of arg. (See MicroBitMessageBus.h).
  */
MicroBitListener::MicroBitListener(uint16_t id, uint16_t value, void* handler, void* arg)
{
	this->id = id;
	this->value = value;
	this->cb = handler;
	this->cb_arg = arg;
	this->next = NULL;
}

/**
  * Constructor. 
  * Create a new Message Bus.
  */
MicroBitMessageBus::MicroBitMessageBus()
{
	this->listeners = NULL;
    this->evt_queue_head = NULL;
    this->evt_queue_tail = NULL;
	this->seq = 0;
}

/**
  * Invokes a callback on a given MicroBitListener
  *
  * Internal wrapper function, used to enable
  * parameterized callbacks through the fiber scheduler.
  */
void async_callback(void *param)
{
	MicroBitListener *listener = (MicroBitListener *)param;
	if (listener->cb_arg != NULL)
		((void (*)(MicroBitEvent, void*))listener->cb)(listener->evt, listener->cb_arg);
	else
		((void (*)(MicroBitEvent))listener->cb)(listener->evt);
}


/**
  * Queue the given event for processing at a later time.
  * Add the given event at the tail of our queue.
  *
  * @param The event to queue. 
  */
void MicroBitMessageBus::queueEvent(MicroBitEvent &evt)
{
    MicroBitEventQueueItem *item = new MicroBitEventQueueItem(evt);

    __disable_irq();
   
    if (evt_queue_tail == NULL)
        evt_queue_head = evt_queue_tail = item;
    else 
        evt_queue_tail->next = item;

    __enable_irq();
}

/**
  * Extract the next event from the front of the event queue (if present).
  * @return 
  *
  * @param The event to queue. 
  */
MicroBitEventQueueItem* MicroBitMessageBus::dequeueEvent()
{
    MicroBitEventQueueItem *item = NULL;

    __disable_irq();
 
    if (evt_queue_head != NULL)
    { 
        item = evt_queue_head;
        evt_queue_head = item->next;

        if (evt_queue_head == NULL)
            evt_queue_tail = NULL;
    }
    
    __enable_irq();

    return item;
}

/**
  * Periodic callback from MicroBit.
  * Process at least one event from the event queue, if it is not empty.
  */  
void MicroBitMessageBus::idleTick()
{
    MicroBitEventQueueItem *item = this->dequeueEvent();

    // Whilst there are events to process, pull them off the queue and process them.
    while (item != NULL)
    {
        // send the event.
        this->send(item->evt);

        // Free the queue item.
        delete item;

        // Pull the next event to process, if there is one.
        item = this->dequeueEvent();
    }
}

/**
  * Indicates whether or not we have any background work to do.
  * @ return 1 if there are any events waitingto be processed, 0 otherwise. 
  */
int MicroBitMessageBus::isIdleCallbackNeeded()
{
    return !(evt_queue_head == NULL);  
}

/**
  * Send the given event to all regstered recipients.
  *
  * @param The event to send. This structure is assumed to be heap allocated, and will 
  * be automatically freed once all recipients have been notified.
  */
void MicroBitMessageBus::send(MicroBitEvent &evt)
{
	this->send(evt, NULL);
}

/**
  * Send the given event to all registered recipients, using a cached entry to minimize lookups.
  * This is particularly useful for optimizing sensors that frequently send to the same channel. 
  *
  * @param evt The event to send. This structure is assumed to be heap allocated, and will 
  * be automatically freed once all recipients have been notified.
  * @param c Cache entry to reduce lookups for commonly used channels.
  *
  * TODO: For now, this is unbuffered. We should consider scheduling events here, and operating
  * a different thread that empties the queue. This would perhaps provide greater opportunities
  * for aggregation.
  *
  * Example:
  * @code 
  * MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_DOWN,ticks,NULL);
  * evt.fire();
  * //OR YOU CAN DO THIS...  
  * MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_DOWN,ticks,NULL,true);
  * @endcode
  *
  * @note THIS IS NOW WRAPPED BY THE MicroBitEvent CLASS FOR CONVENIENCE...
  */
void MicroBitMessageBus::send(MicroBitEvent &evt, MicroBitMessageBusCache *c)
{
	MicroBitListener *l;
	MicroBitListener *start;

    // Firstly, determine if we're operating in interrupt context...
    // If so, we queue processing of the event until we're scheduled in normal thread context.
    // We do this to avoid executing event handler code in IRQ context, which may bring
    // hidden race condition to kids code. 

    if (inInterruptContext())
    {
        this->queueEvent(evt);
        return;
    }

	// Find the start of the sublist where we'll send this event.
	// Ideally, we'll have a valid, cached entry. Use it if we do.
	if ( c != NULL && c->seq == this->seq)
	{
		l = c->ptr;
	}
	else
	{
		l = listeners;
		while (l != NULL && l->id != evt.source)
			l = l->next;
	}

	start = l;

	// Now, send the event to all listeners registered for this event.
	while (l != NULL && l->id == evt.source)
	{
		if(l->value ==  MICROBIT_EVT_ANY || l->value == evt.value)
		{
			l->evt = evt;
			fork_on_block(async_callback, (void *)l);
		}

		l = l->next;
	}

	// Next, send to any listeners registered for ALL event sources.
	l = listeners;
	while (l != NULL && l->id == MICROBIT_ID_ANY)
	{
		l->evt = evt;
		fork_on_block(async_callback, (void *)l);
		
		l = l->next;	
	}

	// If we were given a cached entry that's now invalid, update it.
	if ( c != NULL && c->seq != this->seq)
	{
		c->ptr = start;
		c->seq = this->seq;
	}
	
	// Wake up any fibers that are blocked on this event
	if (uBit.flags & MICROBIT_FLAG_SCHEDULER_RUNNING)
        scheduler_event(evt);  
 
	
	// Finally, see if this event needs to be propogated through our BLE interface 
	if (uBit.ble_event_service)
    	uBit.ble_event_service->onMicroBitEvent(evt);
}

/**
  * Register a listener function.
  * 
  * @param id The source of messages to listen for. Events sent from any other IDs will be filtered. 
  * Use MICROBIT_ID_ANY to receive events from all components.
  *
  * @param value The value of messages to listen for. Events with any other values will be filtered. 
  * Use MICROBIT_VALUE_ANY to receive events of any value.
  *
  * @param hander The function to call when an event is received.
  *
  * TODO: We currently don't support C++ member functions as callbacks, which we should.
  *
  * Example:
  * @code 
  * void onButtonBClick(MicroBitEvent evt)
  * {
  * 	//do something
  * }
  * uBit.MessageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, onButtonBClick); // call function when ever a click event is detected.
  * @endcode
  */

void MicroBitMessageBus::listen(int id, int value, void (*handler)(MicroBitEvent, void*), void* arg) {
	this->listen(id, value, (void*) handler, arg);
}

void MicroBitMessageBus::listen(int id, int value, void (*handler)(MicroBitEvent)) {
	this->listen(id, value, (void*) handler, NULL);
}

void MicroBitMessageBus::listen(int id, int value, void* handler, void* arg)
{
	//handler can't be NULL!
	if (handler == NULL)
		return;

	MicroBitListener *l, *p;

	l = listeners;

	// Firstly, we treat a listener as an idempotent operation. Ensure we don't already have this handler
	// registered in a that will already capture these events. If we do, silently ignore.
	while (l != NULL)
	{
		if (l->id == id && l->value == value && l->cb == handler)
			return;

		l = l->next;
	}

	MicroBitListener *newListener = new MicroBitListener(id, value, handler, arg);

	//if listeners is null - we can automatically add this listener to the list at the beginning...
	if (listeners == NULL)
	{
		listeners = newListener;
		return;
	}

	// Maintain an ordered list of listeners. 
	// Chain is held stictly in increasing order of ID (first level), then value code (second level).
	// Find the correct point in the chain for this event.
	// Adding a listener is a rare occurance, so we just walk the list.
	p = listeners;
	l = listeners;

	while (l != NULL && l->id < id)
	{
		p = l;
		l = l->next;
	}

	while (l != NULL && l->id == id && l->value < value)
	{
		p = l;
		l = l->next;
	}



	//add at front of list
	if (p == listeners && (id < p->id || (p->id == id && p->value > value)))
	{
		newListener->next = p;

		//this new listener is now the front!
		listeners = newListener;
	}
	//add after p
	else
	{
		newListener->next = p->next;
		p->next = newListener;
	}

	// Increase our sequence number and we're done.
	// This will lazily invalidate any cached entries to the listener list.
	this->seq++;
}

