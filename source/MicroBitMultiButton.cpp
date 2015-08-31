#include "MicroBit.h"

void
onMultiButtonEvent(MicroBitEvent evt)
{   
    uBit.buttonAB.onEvent(evt);
}

/**
  * Constructor. 
  * Create a representation of a virtual button, that generates events based upon the combination
  * of two given buttons.
  * @param id the ID of the new MultiButton object.
  * @param button1 the ID of the first button to integrate.
  * @param button2 the ID of the second button to integrate.
  * @param name the physical pin on the processor that this butotn is connected to.
  *
  * Example:
  * @code 
  * multiButton(MICROBIT_ID_BUTTON_AB, MICROBIT_ID_BUTTON_A, MICROBIT_ID_BUTTON_B); 
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
MicroBitMultiButton::MicroBitMultiButton(uint16_t id, uint16_t button1, uint16_t button2)
{
    this->id = id;
    this->button1 = button1;
    this->button2 = button2;
    
    uBit.MessageBus.listen(button1, MICROBIT_EVT_ANY, onMultiButtonEvent);
    uBit.MessageBus.listen(button2, MICROBIT_EVT_ANY, onMultiButtonEvent);
}

uint16_t MicroBitMultiButton::otherSubButton(uint16_t b)
{
    return (b == button1 ? button2 : button1);
}

int MicroBitMultiButton::isSubButtonPressed(uint16_t button)
{
    if (button == button1)
        return status & MICROBIT_MULTI_BUTTON_STATE_1;
        
    if (button == button2)
        return status & MICROBIT_MULTI_BUTTON_STATE_2;
        
    return 0;
}

int MicroBitMultiButton::isSubButtonHeld(uint16_t button)
{
    if (button == button1)
        return status & MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_1;
        
    if (button == button2)
        return status & MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_2;
        
    return 0;
}


void MicroBitMultiButton::setButtonState(uint16_t button, int value)
{
    if (button == button1)
    {
        if (value)
            status |= MICROBIT_MULTI_BUTTON_STATE_1;
        else
            status &= ~MICROBIT_MULTI_BUTTON_STATE_1;
    }
        
    if (button == button2)
    {
        if (value)
            status |= MICROBIT_MULTI_BUTTON_STATE_2;
        else
            status &= ~MICROBIT_MULTI_BUTTON_STATE_2;
    }
}

void MicroBitMultiButton::setHoldState(uint16_t button, int value)
{
    if (button == button1)
    {
        if (value)
            status |= MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_1;
        else
            status &= ~MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_1;
    }
        
    if (button == button2)
    {
        if (value)
            status |= MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_2;
        else
            status &= ~MICROBIT_MULTI_BUTTON_HOLD_TRIGGERED_2;
    }
}

void MicroBitMultiButton::onEvent(MicroBitEvent evt)
{
    int button = evt.source;
    int otherButton = otherSubButton(button);
    
    switch(evt.value)
    {
        case MICROBIT_BUTTON_EVT_DOWN:
            setButtonState(button, 1);
            if(isSubButtonPressed(otherButton))
                MicroBitEvent e(id, MICROBIT_BUTTON_EVT_DOWN);
                
        break;
        
        case MICROBIT_BUTTON_EVT_UP:
            setButtonState(button, 0);
            setHoldState(button, 0);
            
            if(isSubButtonPressed(otherButton))
                MicroBitEvent e(id, MICROBIT_BUTTON_EVT_UP);
        break;
        
        case MICROBIT_BUTTON_EVT_CLICK:
        case MICROBIT_BUTTON_EVT_LONG_CLICK:
            setButtonState(button, 0);
            setHoldState(button, 0);
            if(isSubButtonPressed(otherButton))
                MicroBitEvent e(id, evt.value);
            
        break;
        
        case MICROBIT_BUTTON_EVT_HOLD:
            setHoldState(button, 1);
            if(isSubButtonHeld(otherButton))
                MicroBitEvent e(id, MICROBIT_BUTTON_EVT_HOLD);
            
        break;
    }
}


/**
  * Tests if this MultiButton is currently pressed.
  * @return 1 if both physical buttons are pressed simultaneously.
  */
int MicroBitMultiButton::isPressed()
{
    return ((status & MICROBIT_MULTI_BUTTON_STATE_1) && (status & MICROBIT_MULTI_BUTTON_STATE_2));
}
