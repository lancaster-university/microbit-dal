#ifndef MICROBIT_COMPONENT_H
#define MICROBIT_COMPONENT_H

/**
  * Class definition for MicroBitComponent
  * All components should inherit from this class.
  * @note if a component needs to be called regularly, then you should add the component to the systemTick and idleTick queues.
  * If it's in the systemTick queue, you should override systemTick and implement the required functionality.
  * Similarly if the component is in the idleTick queue, the idleTick member function should be overridden.
  */

// Enumeration of core components.
#define MICROBIT_ID_BUTTON_A            1
#define MICROBIT_ID_BUTTON_B            2
#define MICROBIT_ID_BUTTON_RESET        3
#define MICROBIT_ID_ACCELEROMETER       4
#define MICROBIT_ID_COMPASS             5
#define MICROBIT_ID_DISPLAY             6

//EDGE connector events
#define MICROBIT_IO_PINS                20

#define MICROBIT_ID_IO_P0               7           //P0 is the left most pad (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P1               8           //P1 is the middle pad (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P2               9           //P2 is the right most pad (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P3               10          //COL1 (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P4               11          //BTN_A
#define MICROBIT_ID_IO_P5               12          //COL2 (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P6               13          //ROW2
#define MICROBIT_ID_IO_P7               14          //ROW1
#define MICROBIT_ID_IO_P8               15          //PIN 18
#define MICROBIT_ID_IO_P9               16          //ROW3
#define MICROBIT_ID_IO_P10              17          //COL3 (ANALOG/DIGITAL)
#define MICROBIT_ID_IO_P11              18          //BTN_B
#define MICROBIT_ID_IO_P12              19          //PIN 20
#define MICROBIT_ID_IO_P13              20          //SCK
#define MICROBIT_ID_IO_P14              21          //MISO
#define MICROBIT_ID_IO_P15              22          //MOSI
#define MICROBIT_ID_IO_P16              23          //PIN 16
#define MICROBIT_ID_IO_P19              24          //SCL
#define MICROBIT_ID_IO_P20              25          //SDA

#define MICROBIT_ID_BUTTON_AB           26          // Button A+B multibutton
#define MICROBIT_ID_GESTURE             27          // Gesture events

#define MICROBIT_ID_THERMOMETER         28
#define MICROBIT_ID_RADIO               29
#define MICROBIT_ID_RADIO_DATA_READY    30
#define MICROBIT_ID_MULTIBUTTON_ATTACH  31

#define MICROBIT_ID_MESSAGE_BUS_LISTENER            1021          // Message bus indication that a handler for a given ID has been registered.
#define MICROBIT_ID_NOTIFY_ONE                      1022          // Notfication channel, for general purpose synchronisation
#define MICROBIT_ID_NOTIFY                          1023          // Notfication channel, for general purpose synchronisation

class MicroBitComponent
{
    protected:

    uint16_t id;                    // Event Bus ID
    uint8_t status;                 // keeps track of various component state, and also indicates if data is ready.

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
      * Once added to the systemTickComponents array, this member function will be
      * called in interrupt context on every system tick.
      */
    virtual void systemTick(){

    }

    /**
      * Once added to the idleThreadComponents array, this member function will be
      * called in idle thread context indiscriminately.
      */
    virtual void idleTick()
    {

    }

    /**
      * When added to the idleThreadComponents array, this function will be called to determine
      * if and when data is ready.
      * @note override this if you want to request to be scheduled imminently
      */
    virtual int isIdleCallbackNeeded()
    {
        return 0;
    }

    virtual ~MicroBitComponent()
    {

    }
};

#endif
