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


#include "CalliopeRGB.h"
#include "MicroBitSystemTimer.h"
#include "nrf_gpio.h"

//Default values for the LED color
#define RGB_LED_DEFAULT_GREEN               0
#define RGB_LED_DEFAULT_RED                 0
#define RGB_LED_DEFAULT_BLUE                0
#define RGB_LED_DEFAULT_WHITE               0

// a more accurate delay function, just to make sure
static void inline
__attribute__((__gnu_inline__,__always_inline__))
nrf_delay_us(uint32_t volatile number_of_us)
{
    __ASM volatile (
        "1:\tSUB %0, %0, #1\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "NOP\n\t"
        "BNE 1b\n\t"
        : "+r"(number_of_us)
    );
}
//the following defines are timed specifically to the sending algorithm in CalliopeRGB.cpp
//timings for sending to the RGB LED:
//logical '0': time HIGH: 0.35 us ±150 ns   time LOW: 0.9 us ±150 ns
//logical '1': time HIGH: 0.9 us ±150 ns    time LOW: 0.35 us ±150 ns

CalliopeRGB::CalliopeRGB(uint32_t pin, uint8_t maxBrightness)
{
    outputPin = pin;
    maxIntensity = maxBrightness;

    //init pin
    nrf_gpio_cfg_output(outputPin);
    nrf_gpio_pin_clear(outputPin);
    
    state = 0;
    
    //init color settings
    GRBW[0] = RGB_LED_DEFAULT_GREEN;
    GRBW[1] = RGB_LED_DEFAULT_RED;
    GRBW[2] = RGB_LED_DEFAULT_BLUE;
    GRBW[3] = RGB_LED_DEFAULT_WHITE;
    
    //check intensity
    for(uint8_t i=0; i<4; i++) {
        GRBW[i] = (uint8_t) (GRBW[i] * maxIntensity / 255.0);
    }

    NRF_GPIO->OUTCLR = (1UL << outputPin);
    nrf_delay_us(50);

    off();
    
    //add RGB LED object to the sytem timer
    system_timer_add_component(this);
}

CalliopeRGB::~CalliopeRGB()
{
    system_timer_remove_component(this);
}

void CalliopeRGB::setMaxBrightness(uint8_t max) {
    maxIntensity = max;
}
 
//sets all 4 color settings to the given values        
void CalliopeRGB::setColour(uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
{
    //set color
    GRBW[0] = green;      
    GRBW[1] = red;
    GRBW[2] = blue;
    GRBW[3] = white;

#if CONFIG_ENABLED(MICROBIT_DBG)
    SERIAL_DEBUG->printf("RGB(%d,%d,%d,%d) ->", GRBW[1], GRBW[0],GRBW[2],GRBW[3]);
#endif
    //check intensity
    for(uint8_t i=0; i<4; i++) {
        GRBW[i] = (uint8_t) (GRBW[i] * maxIntensity / 255.0);
    }
#if CONFIG_ENABLED(MICROBIT_DBG)
    SERIAL_DEBUG->printf("RGB(%d,%d,%d,%d)\r\n", GRBW[1], GRBW[0],GRBW[2],GRBW[3]);
#endif
    
    //apply settings
    this->send();
}

void CalliopeRGB::on()
{
    this->send();
}


void CalliopeRGB::off()
{
    //save color settings
    uint8_t GRBW_temp[4];
    for(uint8_t i=0; i<4; i++) GRBW_temp[i] = GRBW[i];
    
    //set all color values to zero and send to LED
    for(uint8_t i=0; i<4; i++) GRBW[i] = 0;
    this->send();
    
    //restore color values
    for(uint8_t i=0; i<4; i++) GRBW[i] = GRBW_temp[i];
}

//sends current color settings to the RGB LED
void CalliopeRGB::send()
{
    //set outputPin to LOW for 50 us
    NRF_GPIO->OUTCLR = (1UL << outputPin);
    nrf_delay_us(50);
    
    //send bytes
    for (uint8_t i=0; i<4; i++)
    {
        for(int8_t j=7; j>-1; j--) 
        {
            if (GRBW[i] & (1 << j))
            {
                //  Timings where off when using GCC and okay using ARMCC with the original
                //  code.
                // 
                //  THE FIX:
                //  Originally all instances of CONST_BIT uses where macro usages of (1UL<<outputPin)
                //  to set or clear the output on that pin. This produced correct timing using
                //  ARMCC, but signals were too long on GCC. James Devine found that introducing
                //  a variable and using it fixed that issue. However, only if used in our main()
                //  program. When moved here, it broke again. Adding a debug printf() statement
                //  between the initial LOW signal and the sending loop fixed it again, but that 
                //  is not a good solution. Inspecting the assembler code, I found that the off
                //  timing was due to an extra LDR instruction issued before the STR to write to 
                //  the OUTSET/OUTCLR.
                //  
                //  By declaring the pin number into a register forces the compiler issue a STR
                //  assembler instruction instead of the LDR + STR sequence, thus fixing the timing.
                // 
                //  I am sure there must be a better solution, but for now it works. A big obstacle
                //  was the use of different compilers for local development and cloud ... 
                register uint32_t CONST_BIT = 1UL << outputPin;
                NRF_GPIO->OUTSET = CONST_BIT;
                __ASM volatile (
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                );
                NRF_GPIO->OUTCLR = CONST_BIT; 
            }
            else
            {
                // put here to force use of a register (see above)
                register uint32_t CONST_BIT = 1UL << outputPin;
                NRF_GPIO->OUTSET = CONST_BIT;
                __ASM volatile ( 
                    "NOP\n\t"
                );
                NRF_GPIO->OUTCLR = CONST_BIT; 
                __ASM volatile ( 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                    "NOP\n\t" 
                ); 
            }
        }
    }
    // set outputPin to LOW for 50 us to latch LEDs
    NRF_GPIO->OUTCLR = (1UL << outputPin);
    nrf_delay_us(50);
    
    //set state
    uint16_t temp = GRBW[0] + GRBW[1] + GRBW[2] + GRBW[3];
    if(temp > 0) state = 1;
    else state = 0;
}

//getter functions for current color settings
uint8_t CalliopeRGB::getRed()
{
    return GRBW[1];   
}

uint8_t CalliopeRGB::getGreen()
{
    return GRBW[0];  
}

uint8_t CalliopeRGB::getBlue()
{
    return GRBW[2];
}

uint8_t CalliopeRGB::getWhite()
{
    return GRBW[3]; 
}

        
//check function
bool CalliopeRGB::isOn()
{
    return state != 0;
}
            
void CalliopeRGB::systemTick()
{
    //currently not in use
}
