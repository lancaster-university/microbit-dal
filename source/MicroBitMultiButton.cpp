#include "MicroBit.h"

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
    
    uBit.MessageBus.listen(button1, MICROBIT_EVT_ANY, this, &MicroBitMultiButton::onButtonEvent,  MESSAGE_BUS_LISTENER_IMMEDIATE);
    uBit.MessageBus.listen(button2, MICROBIT_EVT_ANY, this, &MicroBitMultiButton::onButtonEvent,  MESSAGE_BUS_LISTENER_IMMEDIATE);
    uBit.MessageBus.listen(MICROBIT_ID_MESSAGE_BUS_LISTENER, id, this, &MicroBitMultiButton::onListenerRegisteredEvent,  MESSAGE_BUS_LISTENER_IMMEDIATE);
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

int MicroBitMultiButton::isSubButtonSupressed(uint16_t button)
{
    if (button == button1)
        return status & MICROBIT_MULTI_BUTTON_SUPRESSED_1;
        
    if (button == button2)
        return status & MICROBIT_MULTI_BUTTON_SUPRESSED_2;
        
    return 0;
}

int MicroBitMultiButton::isListenerAttached()
{
   return status & MICROBIT_MULTI_BUTTON_ATTACHED;
}

void MicroBitMultiButton::setListenerAttached(int value)
{
    if (value)
        status |= MICROBIT_MULTI_BUTTON_ATTACHED;
    else
        status &= ~MICROBIT_MULTI_BUTTON_ATTACHED;
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

void MicroBitMultiButton::setSupressedState(uint16_t button, int value)
{
    if (button == button1)
    {
        if (value)
            status |= MICROBIT_MULTI_BUTTON_SUPRESSED_1;
        else
            status &= ~MICROBIT_MULTI_BUTTON_SUPRESSED_1;
    }
        
    if (button == button2)
    {
        if (value)
            status |= MICROBIT_MULTI_BUTTON_SUPRESSED_2;
        else
            status &= ~MICROBIT_MULTI_BUTTON_SUPRESSED_2;
    }
}

void MicroBitMultiButton::onListenerRegisteredEvent(MicroBitEvent evt)
{
    (void) evt;     // Unused parameter

    // Simply indicate to the buttons we are tracking that they are now part of a button group.
    // As a result, they will suppress some individual events from being generated.
    MicroBitEvent(MICROBIT_ID_MULTIBUTTON_ATTACH, button1);
    MicroBitEvent(MICROBIT_ID_MULTIBUTTON_ATTACH, button2);
    setListenerAttached(1);
}

void MicroBitMultiButton::onButtonEvent(MicroBitEvent evt)
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

        case MICROBIT_BUTTON_EVT_HOLD:
            setHoldState(button, 1);
            if(isSubButtonHeld(otherButton))
                MicroBitEvent e(id, MICROBIT_BUTTON_EVT_HOLD);
            
        break;
        
        case MICROBIT_BUTTON_EVT_UP:
            if(isSubButtonPressed(otherButton))
            {
                MicroBitEvent e(id, MICROBIT_BUTTON_EVT_UP);
                
                if (isSubButtonHeld(button) && isSubButtonHeld(otherButton))
                    MicroBitEvent e(id, MICROBIT_BUTTON_EVT_LONG_CLICK);
                else
                    MicroBitEvent e(id, MICROBIT_BUTTON_EVT_CLICK);

                setSupressedState(otherButton, 1);
            }
            else if (!isSubButtonSupressed(button) && isListenerAttached())
            {
                if (isSubButtonHeld(button))
                    MicroBitEvent e(button, MICROBIT_BUTTON_EVT_LONG_CLICK);
                else
                    MicroBitEvent e(button, MICROBIT_BUTTON_EVT_CLICK);
            }

            setButtonState(button, 0);
            setHoldState(button, 0);
            setSupressedState(button, 0);

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
