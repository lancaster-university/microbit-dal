/*
The MIT License (MIT)

Copyright (c) 2016 Calliope GbR
This software is provided by DELTA Systems (Georg Sommer) - Thomas Kern
und Björn Eberhardt GbR by arrangement with Calliope GbR. 

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
#include "MicroBitPin.h"

//max light intensity
#define RGB_LED_MAX_INTENSITY               40
#ifdef TARGET_NRF51_CALLIOPE
#define RGB_DEFAULT_PIN CALLIOPE_PIN_RGB_LED
#else
#define RGB_DEFAULT_PIN PAD1
#endif

class CalliopeRGB : public MicroBitComponent
{   
    uint32_t outputPin;
    uint8_t maxIntensity;

    //values for the displayed color
    uint8_t GRBW[4];
    
    //current state. 0 = off, 1 = on
    uint8_t state;
        
    public:
        //constructor
        CalliopeRGB(uint32_t pin = RGB_DEFAULT_PIN, uint8_t maxBrightness = RGB_LED_MAX_INTENSITY);
        
        //destructor
        ~CalliopeRGB(); 

        void setMaxBrightness(uint8_t max);

        //functions to control the the LED
        //sets all 4 color settings to the given values  
        void setColour(uint8_t red, uint8_t green, uint8_t blue, uint8_t white);
        void on();
        void off();
        
        //getter functions
        uint8_t getRed();
        uint8_t getGreen();
        uint8_t getBlue();
        uint8_t getWhite();
        
        //check function
        bool isOn();
        
        //periodic callback from MicroBit system timer.
        virtual void systemTick();

    protected:
        void send();

};

#endif
