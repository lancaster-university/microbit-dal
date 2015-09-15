#ifndef MICROBIT_BUTTON_H
#define MICROBIT_BUTTON_H

#include "mbed.h"
#include "MicroBitComponent.h"
#include "MicroBitEvent.h"

//TODO: When platform is built for MB2 - pins will be defined by default, these will change...
#define MICROBIT_PIN_BUTTON_A                   P0_17
#define MICROBIT_PIN_BUTTON_B                   P0_26
#define MICROBIT_PIN_BUTTON_RESET               P0_19

#define MICROBIT_BUTTON_EVT_DOWN                1
#define MICROBIT_BUTTON_EVT_UP                  2
#define MICROBIT_BUTTON_EVT_CLICK               3
#define MICROBIT_BUTTON_EVT_LONG_CLICK          4
#define MICROBIT_BUTTON_EVT_HOLD                5
#define MICROBIT_BUTTON_EVT_DOUBLE_CLICK        6

#define MICROBIT_BUTTON_LONG_CLICK_TIME         1000  
#define MICROBIT_BUTTON_HOLD_TIME               1500  

#define MICROBIT_BUTTON_STATE                   1
#define MICROBIT_BUTTON_STATE_HOLD_TRIGGERED    2
#define MICROBIT_BUTTON_STATE_CLICK             4
#define MICROBIT_BUTTON_STATE_LONG_CLICK        8

#define MICROBIT_BUTTON_SIGMA_MIN               0
#define MICROBIT_BUTTON_SIGMA_MAX               12
#define MICROBIT_BUTTON_SIGMA_THRESH_HI         8
#define MICROBIT_BUTTON_SIGMA_THRESH_LO         2
#define MICROBIT_BUTTON_DOUBLE_CLICK_THRESH     50

/**
  * Class definition for MicroBit Button.
  *
  * Represents a single, generic button on the device.
  */
class MicroBitButton : public MicroBitComponent
{
    PinName name;                   // mBed pin name of this pin.
    DigitalIn pin;                  // The mBed object looking after this pin at any point in time (may change!).
    
    unsigned long downStartTime;    // used to store the current system clock when a button down event occurs
    uint8_t sigma;                  // integration of samples over time.
    uint8_t doubleClickTimer;       // double click timer (ticks).
    
    public:

    /**
      * Constructor. 
      * Create a pin representation with the given ID.
      * @param id the ID of the new MicroBitButton object.
      * @param name the physical pin on the processor that this butotn is connected to.
      * @param mode the configuration of internal pullups/pulldowns, as define in the mbed PinMode class. PullNone by default.
      *
      * Example:
      * @code 
      * buttonA(MICROBIT_ID_BUTTON_A,MICROBIT_PIN_BUTTON_A); //a number between 0 and 200 inclusive
      * @endcode
      *
      * Possible Events:
      * @code 
      * MICROBIT_BUTTON_EVT_DOWN
      * MICROBIT_BUTTON_EVT_UP
      * MICROBIT_BUTTON_EVT_CLICK
      * MICROBIT_BUTTON_EVT_LONG_CLICK
      * MICROBIT_BUTTON_EVT_DOUBLE_CLICK
      * MICROBIT_BUTTON_EVT_HOLD
      * @endcode
      */
    MicroBitButton(uint16_t id, PinName name, PinMode mode = PullNone);
    
    /**
      * Tests if this Button is currently pressed.
      * @return 1 if this button is pressed, 0 otherwise.
      *
      * Example:
      * @code 
      * if(uBit.buttonA.isPressed())
      *     print("Pressed!");
      * @endcode
      */
    int isPressed();
    
    /**
      * periodic callback from MicroBit clock.
      * Check for state change for this button, and fires a hold event if button is pressed.
      */  
    virtual void systemTick();
    
};

#endif
