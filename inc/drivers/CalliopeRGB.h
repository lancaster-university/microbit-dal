/*
The MIT License (MIT)

Copyright (c) 2016 Calliope GbR
This software is provided by DELTA Systems - Thomas Kern und Björn 
Eberhardt GbR by arrangement with Calliope GbR. 

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


#ifndef CALLIOPE_RGB_H
#define CALLIOPE_RGB_H

#include "mbed.h"
#include "MicroBitComponent.h"

//Pin of RGB LED on the MicroBit
#define CALLIOPE_PIN_RGB                    P0_18

//assembler code: do nothing
#define RGB_WAIT                            " NOP\n\t"

//Default values for the LED color
#define RGB_LED_DEFAULT_GREEN               0
#define RGB_LED_DEFAULT_RED                 0
#define RGB_LED_DEFAULT_BLUE                0
#define RGB_LED_DEFAULT_WHITE               0

//max light intensity
#define RGB_LED_MAX_INTENSITY               50

//the following defines are timed specifically to the sending algorithm in CalliopeRGB.cpp
//timings for sending to the RGB LED: 
//logical '0': time HIGH: 0.35 us ±150 ns   time LOW: 0.9 us ±150 ns 
//logical '1': time HIGH: 0.9 us ±150 ns    time LOW: 0.35 us ±150 ns

//sends a logical '1' to the receiver
#define CALLIOPE_RGB_SEND_HIGH  NRF_GPIO->OUTSET = (1UL << PIN); \
    __ASM ( \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
    ); \
    NRF_GPIO->OUTCLR = (1UL << PIN);


//sends a logical '0' to the receiver
#define CALLIOPE_RGB_SEND_LOW   NRF_GPIO->OUTSET = (1UL << PIN); \
    __ASM (  \
        RGB_WAIT \
    );  \
    NRF_GPIO->OUTCLR = (1UL << PIN);  \
    __ASM ( \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
        RGB_WAIT \
    );


class CalliopeRGB : public MicroBitComponent
{   
    //pin object for data transfer                                            
    DigitalOut pin;      
    
    //values for the displayed color
    uint8_t GRBW[4];
    
    //current state. 0 = off, 1 = on
    uint8_t state;
        
    public:
        //constructor
        CalliopeRGB();
        
        //destructor
        ~CalliopeRGB(); 
        
        //functions to control the the LED
        //sets all 4 color settings to the given values  
        void Set_Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white);
        //sets all 4 color settings to the given values or leaves them as they are if no value is given
        void Set_Color2(int red = -1, int green = -1, int blue = -1, int white = -1);
        void On();
        void Off();
        void Send_to_LED();
        
        //getter functions
        uint8_t Get_Red();
        uint8_t Get_Green();
        uint8_t Get_Blue();
        uint8_t Get_White();
        
        //check function
        bool Is_On();
        
        //periodic callback from MicroBit system timer.
        virtual void systemTick();
};

#endif