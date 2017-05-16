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

#ifndef CALLIOPE_MICROPHONE_H
#define CALLIOPE_MICROPHONE_H

#include "mbed.h"
#include "MicroBitIO.h"
#include "MicroBitComponent.h"
// #include "DynamicPwm.h"


#define CALLIOPE_DEFAULT_SAMPLE_RATE                8000        //default sample rate for PWM sampling
#define CALLIOPE_MAX_SAMPLE_RATE                    11025       //max sample rate, limited by sample loop exec time
#define CALLIOPE_MIN_SAMPLE_RATE                    1           //min sample rate
#define CALLIOPE_MIN_SAMPLE_BUFFER_SIZE             1           //min sample buffer size
#define CALLIOPE_SAMPLING_THRESHOLD_UPPER           524         //threshold for registering output as "1"
#define CALLIOPE_SAMPLING_THRESHOLD_LOWER           515         //threshold for registering output as "0"

class CalliopeMicrophone : public MicroBitComponent
{    
    public:
        //constructor
        CalliopeMicrophone();
        
        //destructor
        ~CalliopeMicrophone();
	
	//function to initialize recording
        static void recordSample(uint8_t* buffer, int16_t len, int16_t sample_rate = CALLIOPE_DEFAULT_SAMPLE_RATE);
	//function to stop/abort recording
	static void stopRecording();
	//interrupt service, do not call directly!
	static void updateInput();
	//function check recording status
	static bool isRecording();
	
    private:
        static uint8_t* rec_buffer;
	static int16_t rec_len;
	static int16_t rec_pos;
	static uint8_t pwm_tick;
	static AnalogIn micpin;
	static mbed::Ticker rec_ticker;
        static bool active;
};

#endif
#endif // TARGET_NRF51_CALLIOPE
