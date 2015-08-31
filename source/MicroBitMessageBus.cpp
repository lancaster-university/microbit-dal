/**
  * Class definition for a MicroBitMessageBus.
  *
  * The MicroBitMessageBus handles all messages passed between components.
  */

#include "MicroBit.h"

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
}

/**
  * Invokes a callback on a given MicroBitListener
  *
  * Internal wrapper function, used to enable
  * parameterised callbacks through the fiber scheduler.
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
  * We then continue processing events until something appears on the runqueue.
  */  
void MicroBitMessageBus::idleTick()
{
    MicroBitEventQueueItem *item = this->dequeueEvent();

    // Whilst there are events to process and we have no useful other work to do, pull them off the queue and process them.
    while (item)
    {
        // send the event.
        this->process(item->evt);

        // Free the queue item.
        delete item;

        // If we have created some useful work to do, we stop processing.
        // This helps to minimise the number of blocked fibers we create at any point in time, therefore
        // also reducing the RAM footprint. 
        if(!scheduler_runqueue_empty())
            break;

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
void MicroBitMessageBus::send(MicroBitEvent evt)
{
    // We simply queue processing of the event until we're scheduled in normal thread context.
    // We do this to avoid the possibility of executing event handler code in IRQ context, which may bring
    // hidden race conditions to kids code. Queuing all events ensures causal ordering (total ordering in fact).

    this->queueEvent(evt);
    return;
}

/*
 * Deliver the given event to all registered event handlers.
 * Event handlers are called using the invoke() mechanism provided by the fier scheduler
 * This will attempt to call the event handler directly, but spawn a fiber should that
 * event handler attempt a blocking operation.
 * @param evt The event to be delivered.
 */
void MicroBitMessageBus::process(MicroBitEvent evt)
{
	MicroBitListener *l;

	// Find the start of the sublist where we'll send this event.
    l = listeners;
    while (l != NULL && l->id != evt.source)
        l = l->next;

	// Now, send the event to all listeners registered for this event.
	while (l != NULL && l->id == evt.source)
	{
		if(l->value ==  MICROBIT_EVT_ANY || l->value == evt.value)
		{
			l->evt = evt;
			invoke(async_callback, (void *)l);
		}

		l = l->next;
	}

	// Next, send to any listeners registered for ALL event sources.
	l = listeners;
	while (l != NULL && l->id == MICROBIT_ID_ANY)
	{
		l->evt = evt;
		invoke(async_callback, (void *)l);
		
		l = l->next;	
	}

    // Finally, forward the event to any other internal subsystems that may be interested.
    // We *could* do this through the message bus of course, but this saves additional RAM,
    // and procssor time (as we know these are non-blocking calls).

	// Wake up any fibers that are blocked on this event
	if (uBit.flags & MICROBIT_FLAG_SCHEDULER_RUNNING)
        scheduler_event(evt);  
 
	// See if this event needs to be propogated through our BLE interface 
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
}

