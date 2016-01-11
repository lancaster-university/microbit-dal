#include "MicroBit.h"

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
  * MICROBIT_BUTTON_EVT_HOLD
  * @endcode
  */
MicroBitButton::MicroBitButton(uint16_t id, PinName name, MicroBitButtonEventConfiguration eventConfiguration) : DebouncedPin(name)
{
    this->id = id;
    this->eventConfiguration = eventConfiguration;
    this->downStartTime = 0;
    this->sigma = 0;
    uBit.addSystemComponent(this);
}

/**
  * periodic callback from MicroBit clock.
  * Check for state change for this button, and fires a hold event if button is pressed.
  */  
void MicroBitButton::systemTick()
{   
    PinTransition transition = this->tick();
    switch (transition) {
        case LOW_LOW:
            if(!(status & MICROBIT_BUTTON_STATE_HOLD_TRIGGERED) && (ticks - downStartTime) >= MICROBIT_BUTTON_HOLD_TIME) {
                //set the hold triggered event flag
                status |= MICROBIT_BUTTON_STATE_HOLD_TRIGGERED;
                //fire hold event
                MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_HOLD);
            }
            break;
        case LOW_HIGH:
        {
            status = 0;
            MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_UP);
            if((ticks - downStartTime) >= MICROBIT_BUTTON_LONG_CLICK_TIME)
                MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_LONG_CLICK);
            else
                MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_CLICK);
            break;
        }
        case HIGH_LOW:
        {
            status |= MICROBIT_BUTTON_STATE;
            MicroBitEvent evt(id,MICROBIT_BUTTON_EVT_DOWN);
            downStartTime=ticks;
            break;
        }
        case HIGH_HIGH:
            /* Not interesting */
            break;
    };
}

/**
  * Tests if this Button is currently pressed.
  * @return 1 if this button is pressed, 0 otherwise.
  */
int MicroBitButton::isPressed()
{
    return status & MICROBIT_BUTTON_STATE ? 1 : 0;
}
