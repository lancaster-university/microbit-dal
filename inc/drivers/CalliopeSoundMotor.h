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


#ifndef CALLIOPE_SOUND_MOTOR_H
#define CALLIOPE_SOUND_MOTOR_H

#include "mbed.h"
#include "MicroBitComponent.h"
#include "DynamicPwm.h"

//pin definitions
#define CALLIOPE_SM_PIN_NSLEEP                          P0_28
#define CALLIOPE_SM_PIN_IN1                             P0_29
#define CALLIOPE_SM_PIN_IN2                             P0_30

//default values
#define CALLIOPE_SM_DEFAULT_DUTY_M                      50
#define CALLIOPE_SM_DEFAULT_DUTY_S                      100
#define CALLIOPE_SM_DEFAULT_FREQUENCY_S                 4000
#define CALLIOPE_SM_DEFAULT_SILENT_MODE                 1

//constants
#define CALLIOPE_SM_PRESCALER_M                         2
#define CALLIOPE_SM_PRESCALER_S                         0
#define CALLIOPE_SM_PRESCALER_S_LF                      4           //prescaler for creating low frequencies
#define CALLIOPE_SM_PERIOD_M                            100
#define CALLIOPE_MIN_FREQUENCY_HZ_S_NP                  245         //min possible frequency due to 16bit timer resolution (without prescaler)
#define CALLIOPE_MIN_FREQUENCY_HZ_S                     20          //min human audible frequency
#define CALLIOPE_MAX_FREQUENCY_HZ_S                     20000       //max human audible frequency 
#define CALLIOPE_BOARD_FREQUENCY                        16000000

class CalliopeSoundMotor : public MicroBitComponent
{   
    //current settings
    static int8_t duty_motor_percent;
    static uint8_t duty_motor_A_percent;
    static uint8_t duty_motor_B_percent;
    //current dual motor use -> 0: off, 1: motor A on, 2: motor B on, 3: motor A and motor B on
    static uint8_t motor_AB_current_use;
    static uint16_t frequency_sound_hz;
    static bool silent_mode;
    
    //current use of the controller -> 0: off, 1: motor use, 2: dual motor use, 3: sound use
    static uint8_t mode;
    
    public:
        //constructor
        CalliopeSoundMotor();
        
        //destructor
        ~CalliopeSoundMotor();
        
        //functions to control the PWM
        void PWM_init();
        
        //functions to control the motor in single motor use
        //negative input for Motor_On() function -> go backwards
        void Motor_On(int8_t duty_percent = duty_motor_percent);
        void Motor_Coast();
        void Motor_Break();
        void Motor_Sleep();
        
        //functions to control the motors in dual motor use
        void MotorA_On(uint8_t duty_percent = duty_motor_A_percent);
        void MotorB_On(uint8_t duty_percent = duty_motor_B_percent);
        void MotorA_Off();
        void MotorB_Off();
        
        //functions to control the sound
        void Sound_On(uint16_t frequency_hz = frequency_sound_hz);
        void Sound_Set_Silent_Mode(bool on_off);
        void Sound_Off();
        
        //check fucntions
        bool Motor_Is_On();
        bool Sound_Is_On();
        
        //getter functions -> will return current settings even if motor/sound is off
        uint8_t Get_Mode();
        int8_t Get_Duty_Motor();
        uint16_t Get_Frequency_Sound();
        
        //periodic callback from MicroBit system timer.
        virtual void systemTick();
};

#endif
