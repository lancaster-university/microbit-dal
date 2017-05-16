/*
The MIT License (MIT)

Copyright (c) 2017 Calliope gGmbH
This software is provided by utz (M. Neidel) by arrangement with Calliope gGmbH.

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


#ifdef TARGET_NRF51_CALLIOPE

#include "CalliopeMicrophone.h"
#include "MicroBitSystemTimer.h"
#include "MicroBitPin.h"

//define static members
uint8_t* CalliopeMicrophone::rec_buffer;
int16_t CalliopeMicrophone::rec_len;
int16_t CalliopeMicrophone::rec_pos;
uint8_t CalliopeMicrophone::pwm_tick;
AnalogIn CalliopeMicrophone::micpin(MIC);
mbed::Ticker CalliopeMicrophone::rec_ticker;
bool CalliopeMicrophone::active = false;

//constructor
CalliopeMicrophone::CalliopeMicrophone()
{
   system_timer_add_component(this);
}


//destructor
CalliopeMicrophone::~CalliopeMicrophone()
{
   system_timer_remove_component(this);
}


//PWM sampling function: records sound from microphone and converts to 1-bit PWM data until buffer is full
void CalliopeMicrophone::recordSample(uint8_t* buffer, int16_t len, int16_t sample_rate)
{
    //refuse to run if already recording
    if (active) return;
    
    //return on invalid parameters
    if (len < CALLIOPE_MIN_SAMPLE_BUFFER_SIZE || sample_rate > CALLIOPE_MAX_SAMPLE_RATE 
	|| sample_rate < CALLIOPE_MIN_SAMPLE_RATE) return;
    
    //set recording mode
    active = true;
    
    //initialize recording parameters
    rec_buffer = buffer;
    rec_len = len;
    rec_pos = 0;
    pwm_tick = 1;
    
    //set up interrupt service
    rec_ticker.attach_us(&updateInput, static_cast<timestamp_t>(1000000 / sample_rate));
}


void CalliopeMicrophone::stopRecording() {

    //do nothing if not recording
    if (!active) return;

    //disable interrupt   
    rec_ticker.detach();
    
    //set recording status
    active = false;
}

//interrupt service routine for sample recording - do not call directly!
void CalliopeMicrophone::updateInput()
{
    //stop recording if buffer is full
    if (rec_pos >= rec_len)
    {
        stopRecording();
        return;
    }

    //read value analog input
    uint16_t val = micpin.read_u16();
    
    //update pwm period counter and write to buffer if input crossed threshold or counter wrapped
    if (pwm_tick && ((!(rec_pos & 1) && val > CALLIOPE_SAMPLING_THRESHOLD_LOWER) 
	|| ((rec_pos & 1) && val < CALLIOPE_SAMPLING_THRESHOLD_UPPER))) ++pwm_tick;
    else
    {
        rec_buffer[rec_pos] = pwm_tick;
	pwm_tick = 1;
	++rec_pos;
    }
}

//function for checking recording status
bool CalliopeMicrophone::isRecording()
{
	return active;
}

#endif
