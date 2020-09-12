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

#ifndef MICROBIT_COMPONENT_H
#define MICROBIT_COMPONENT_H

#include "MicroBitConfig.h"

// Enumeration of core components.
#define MICROBIT_ID_BUTTON_A            1
#define MICROBIT_ID_BUTTON_B            2
#define MICROBIT_ID_BUTTON_AB           3           // Button A+B multibutton
#define MICROBIT_ID_BUTTON_RESET        4
#define MICROBIT_ID_ACCELEROMETER       5
#define MICROBIT_ID_COMPASS             6
#define MICROBIT_ID_DISPLAY             7
#define MICROBIT_ID_THERMOMETER         8
#define MICROBIT_ID_RADIO               9
#define MICROBIT_ID_RADIO_DATA_READY    10
#define MICROBIT_ID_MULTIBUTTON_ATTACH  11
#define MICROBIT_ID_SERIAL              12
#define MICROBIT_ID_GESTURE             13          // Gesture events

#define MICROBIT_ID_IO_P0               100         //P0 is the left most pad (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P1               101         //P1 is the middle pad (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P2               102         //P2 is the right most pad (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P3               103         //COL1 (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P4               104         //BTN_A
#define MICROBIT_ID_IO_P5               105         //COL2 (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P6               106         //ROW2
#define MICROBIT_ID_IO_P7               107         //ROW1
#define MICROBIT_ID_IO_P8               108         //PIN 18
#define MICROBIT_ID_IO_P9               109         //ROW3
#define MICROBIT_ID_IO_P10              110         //COL3 (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P11              111         //BTN_B
#define MICROBIT_ID_IO_P12              112         //PIN 20
#define MICROBIT_ID_IO_P13              113         //SCK
#define MICROBIT_ID_IO_P14              114         //MISO
#define MICROBIT_ID_IO_P15              115         //MOSI
#define MICROBIT_ID_IO_P16              116         //PIN 16
#define MICROBIT_ID_IO_P19              119         //SCL
#define MICROBIT_ID_IO_P20              120         //SDA

#define MICROBIT_ID_IO_INT1             130         //INT1
#define MICROBIT_ID_IO_INT2             131         //INT2
#define MICROBIT_ID_IO_INT3             132         //INT3

// System Softwarre components
#define MICROBIT_ID_PARTIAL_FLASHING                200

#define MICROBIT_ID_MESSAGE_BUS_LISTENER            1021          // Message bus indication that a handler for a given ID has been registered.
#define MICROBIT_ID_NOTIFY_ONE                      1022          // Notfication channel, for general purpose synchronisation
#define MICROBIT_ID_NOTIFY                          1023          // Notfication channel, for general purpose synchronisation

// Universal flags used as part of the status field
#define MICROBIT_COMPONENT_RUNNING		0x01


/**
  * Class definition for MicroBitComponent.
  *
  * All components should inherit from this class.
  *
  * If a component requires regular updates, then that component can be added to the 
  * to the systemTick and/or idleTick queues. This provides a simple, extensible mechanism
  * for code that requires periodic/occasional background processing but does not warrant
  * the complexity of maintaining its own thread. 
  *
  * Two levels of support are available. 
  *
  * systemTick() provides a periodic callback during the
  * micro:bit's system timer interrupt. This provides a guaranteed periodic callback, but in interrupt context
  * and is suitable for code with lightweight processing requirements, but strict time constraints.
  * 
  * idleTick() provides a periodic callback whenever the scheduler is idle. This provides occasional, callbacks
  * in the main thread context, but with no guarantees of frequency. This is suitable for non-urgent background tasks.
  *
  * Components wishing to use these facilities should override the systemTick and/or idleTick functions defined here, and
  * register their components using system_timer_add_component() fiber_add_idle_component() respectively.
  *
  */
class MicroBitComponent
{
    protected:

    uint16_t id;                    // Event Bus ID of this component
    uint8_t status;                 // Component defined state.

    public:

    /**
      * The default constructor of a MicroBitComponent
      */
    MicroBitComponent()
    {
        this->id = 0;
        this->status = 0;
    }

    /**
      * The system timer will call this member function once the component has been added to
      * the array of system components using system_timer_add_component. This callback
      * will be in interrupt context.
      */
    virtual void systemTick()
    {
    }

    /**
      * The idle thread will call this member function once the component has been added to the array
      * of idle components using fiber_add_idle_component. 
      */
    virtual void idleTick()
    {
    }

    /**
      * If you have added your component to the idle or system tick component arrays,
      * you must remember to remove your component from them if your component is destructed.
      */
    virtual ~MicroBitComponent()
    {
    }
};

#endif
