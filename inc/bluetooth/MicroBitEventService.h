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

#ifndef MICROBIT_EVENT_SERVICE_H
#define MICROBIT_EVENT_SERVICE_H

#include "MicroBitConfig.h"
#include "ble/BLE.h"
#include "MicroBitEvent.h"
#include "EventModel.h"

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitEventServiceUUID[];
extern const uint8_t  MicroBitEventServiceMicroBitEventCharacteristicUUID[];
extern const uint8_t  MicroBitEventServiceClientEventCharacteristicUUID[];
extern const uint8_t  MicroBitEventServiceMicroBitRequirementsCharacteristicUUID[];
extern const uint8_t  MicroBitEventServiceClientRequirementsCharacteristicUUID[];

struct EventServiceEvent
{
    uint16_t    type;
    uint16_t    reason;
};


/**
  * Class definition for a MicroBit BLE Event Service.
  * Provides a BLE gateway onto an Event Model.
  */
class MicroBitEventService : public MicroBitComponent
{
    public:

    /**
      * Constructor.
      * Create a representation of the EventService
      * @param _ble The instance of a BLE device that we're running on.
      * @param _messageBus An instance of an EventModel which events will be mirrored from.
      */
    MicroBitEventService(BLEDevice &_ble, EventModel &_messageBus);

    /**
     * Periodic callback from MicroBit scheduler.
     * If we're no longer connected, remove any registered Message Bus listeners.
     */
    virtual void idleTick();

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);

    /**
      * Callback. Invoked when any events are sent on the microBit message bus.
      */
    void onMicroBitEvent(MicroBitEvent evt);

    /**
      * Read callback on microBitRequirements characteristic.
      *
      * Used to iterate through the events that the code on this micro:bit is interested in.
      */
    void onRequirementsRead(GattReadAuthCallbackParams *params);

    private:

    // Bluetooth stack we're running on.
    BLEDevice           &ble;
	EventModel	        &messageBus;

    // memory for our event characteristics.
    EventServiceEvent   clientEventBuffer;
    EventServiceEvent   microBitEventBuffer;
    EventServiceEvent   microBitRequirementsBuffer;
    EventServiceEvent   clientRequirementsBuffer;

    // handles on this service's characterisitics.
    GattAttribute::Handle_t microBitEventCharacteristicHandle;
    GattAttribute::Handle_t clientRequirementsCharacteristicHandle;
    GattAttribute::Handle_t clientEventCharacteristicHandle;
    GattCharacteristic *microBitRequirementsCharacteristic;

    // Message bus offset last sent to the client...
    uint16_t messageBusListenerOffset;

};


#endif
