#ifndef MICROBIT_PIN_H
#define MICROBIT_PIN_H

#include "mbed.h"
#include "MicroBitComponent.h"
                                                     // Status Field flags...
#define IO_STATUS_DIGITAL_IN             0x01        // Pin is configured as a digital input, with no pull up.
#define IO_STATUS_DIGITAL_OUT            0x02        // Pin is configured as a digital output
#define IO_STATUS_ANALOG_IN              0x04        // Pin is Analog in 
#define IO_STATUS_ANALOG_OUT             0x08        // Pin is Analog out (not currently possible)  
#define IO_STATUS_TOUCH_IN               0x10        // Pin is a makey-makey style touch sensor
#define IO_STATUS_EVENTBUS_ENABLED       0x80        // Pin is will generate events on change

//#defines for each edge connector pin
#define MICROBIT_PIN_P0                  P0_3        //P0 is the left most pad (ANALOG/DIGITAL) used to be P0_3 on green board
#define MICROBIT_PIN_P1                  P0_2        //P1 is the middle pad (ANALOG/DIGITAL) 
#define MICROBIT_PIN_P2                  P0_1        //P2 is the right most pad (ANALOG/DIGITAL) used to be P0_1 on green board
#define MICROBIT_PIN_P3                  P0_4        //COL1 (ANALOG/DIGITAL) 
#define MICROBIT_PIN_P4                  P0_17       //BTN_A           
#define MICROBIT_PIN_P5                  P0_5        //COL2 (ANALOG/DIGITAL) 
#define MICROBIT_PIN_P6                  P0_14       //ROW2
#define MICROBIT_PIN_P7                  P0_13       //ROW1       
#define MICROBIT_PIN_P8                  P0_18       //PIN 18
#define MICROBIT_PIN_P9                  P0_15       //ROW3                  
#define MICROBIT_PIN_P10                 P0_6        //COL3 (ANALOG/DIGITAL) 
#define MICROBIT_PIN_P11                 P0_26       //BTN_B
#define MICROBIT_PIN_P12                 P0_20       //PIN 20
#define MICROBIT_PIN_P13                 P0_23       //SCK
#define MICROBIT_PIN_P14                 P0_22       //MISO
#define MICROBIT_PIN_P15                 P0_21       //MOSI
#define MICROBIT_PIN_P16                 P0_16       //PIN 16
#define MICROBIT_PIN_P19                 P0_0        //SCL
#define MICROBIT_PIN_P20                 P0_30       //SDA

#define MICROBIT_PIN_MAX_OUTPUT          1023


/**
  * Pin capabilities enum. 
  * Used to determine the capabilities of each Pin as some can only be digital, or can be both digital and analogue.
  */
enum PinCapability{
    PIN_CAPABILITY_DIGITAL = 0x01,
    PIN_CAPABILITY_ANALOG = 0x02,
    PIN_CAPABILITY_TOUCH = 0x04,
    PIN_CAPABILITY_AD = PIN_CAPABILITY_DIGITAL | PIN_CAPABILITY_ANALOG,
    PIN_CAPABILITY_ALL = PIN_CAPABILITY_DIGITAL | PIN_CAPABILITY_ANALOG | PIN_CAPABILITY_TOUCH
    
};

/**
  * Class definition for MicroBitPin.
  *
  * Represents a I/O on the edge connector.
  */
class MicroBitPin : public MicroBitComponent
{
    /**
      * Unique, enumerated ID for this component. 
      * Used to track asynchronous events in the event bus.
      */
      
    void *pin;                  // The mBed object looking after this pin at any point in time (may change!).
    PinCapability capability;
   
    /**
      * Disconnect any attached mBed IO from this pin.
      * Used only when pin changes mode (i.e. Input/Output/Analog/Digital)
      */
    void disconnect();
    
    public:
    PinName name;               // mBed pin name of this pin.
    
    /**
      * Constructor. 
      * Create a Button representation with the given ID.
      * @param id the ID of the new Pin object.
      * @param name the pin name for this MicroBitPin instance to represent
      * @param capability the capability of this pin, can it only be digital? can it only be analog? can it be both?
      * 
      * Example:
      * @code 
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * @endcode
      */
    MicroBitPin(int id, PinName name, PinCapability capability);
    
    /**
      * Configures this IO pin as a digital output (if necessary) and sets the pin to 'value'.
      * @param value 0 (LO) or 1 (HI)
      * 
      * Example:
      * @code 
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * P0.setDigitalValue(1); // P0 is now HI!
      * @endcode
      */
    void setDigitalValue(int value);
    
    /**
      * Configures this IO pin as a digital input (if necessary) and tests its current value.
      * @return 1 if this input is high, 0 otherwise.
      * 
      * Example:
      * @code 
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * P0.getDigitalValue(); // P0 is either 0 or 1;
      * @endcode
      */
    int getDigitalValue();

    /**
      * Configures this IO pin as an analogue output (if necessary and possible).
      * Change the DAC value to the given level.
      * @param value the level to set on the output pin, in the range 0..???
      * @note NOT IN USE, but may exist in the future if we do some clever rejigging! :)
      */
    void setAnalogValue(int value);


    /**
      * Configures this IO pin as an analogue input (if necessary and possible).
      * @return the current analogue level on the pin, in the range 0-0xFFFF
      * 
      * Example:
      * @code 
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_BOTH);
      * P0.getAnalogValue(); // P0 is a value in the range of 0 - 0xFFFF
      * @endcode
      */
    int getAnalogValue();

    /**
      * Configures this IO pin as a makey makey style touch sensor (if necessary) and tests its current debounced state.
      * @return 1 if pin is touched, 0 otherwise.
      * 
      * Example:
      * @code 
      * MicroBitPin P0(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ALL);
      * if(P0.isTouched())
      * {
      *   uBit.display.clear();
      * } 
      * @endcode
      */
    int isTouched();

     /**
      * Configures the PWM period of the analog output to the given value.
      * If this pin is not configured as an analog output, the operation
      * has no effect.
      *
      * @param period The new period for the analog output in milliseconds.
      */   
    void setAnalogPeriod(int period);
};

#endif
