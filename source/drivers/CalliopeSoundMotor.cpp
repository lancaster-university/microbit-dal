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


#ifdef TARGET_NRF51_CALLIOPE

#include "CalliopeSoundMotor.h"
#include "MicroBitSystemTimer.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"


//define static members
int8_t CalliopeSoundMotor::duty_motor_percent;
uint8_t CalliopeSoundMotor::duty_motor_A_percent;
uint8_t CalliopeSoundMotor::duty_motor_B_percent;
uint8_t CalliopeSoundMotor::motor_AB_current_use;
uint16_t CalliopeSoundMotor::frequency_sound_hz;
bool CalliopeSoundMotor::silent_mode;
uint8_t CalliopeSoundMotor::mode;


//constructor
CalliopeSoundMotor::CalliopeSoundMotor()
{
    //init sleep pin
    nrf_gpio_cfg_output(CALLIOPE_PIN_MOTOR_SLEEP);
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_SLEEP);

    //set default values
    duty_motor_percent = CALLIOPE_SM_DEFAULT_DUTY_M;
    duty_motor_A_percent = CALLIOPE_SM_DEFAULT_DUTY_M;
    duty_motor_B_percent = CALLIOPE_SM_DEFAULT_DUTY_M;
    motor_AB_current_use = 0;
    frequency_sound_hz = CALLIOPE_SM_DEFAULT_FREQUENCY_S;
    silent_mode = CALLIOPE_SM_DEFAULT_SILENT_MODE;
    mode = 0;

    //init PWM
    this->PWM_init();

    //add RGB LED object to the sytem timer
    system_timer_add_component(this);
}


//destructor
CalliopeSoundMotor::~CalliopeSoundMotor()
{
    system_timer_remove_component(this);
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//PWM INIT
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//function to init the PWM -> results in a PWM signal and an inverted PWM signal on pins CALLIOPE_PIN_MOTOR_IN2 and CALLIOPE_PIN_MOTOR_IN1
void CalliopeSoundMotor::PWM_init()
{
    //init pins
    nrf_gpio_cfg_output(CALLIOPE_PIN_MOTOR_IN1);
    nrf_gpio_cfg_output(CALLIOPE_PIN_MOTOR_IN2);
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);

    //create tasks to perform on timer compare match
    NRF_GPIOTE->POWER = 1;
    //task 0
    nrf_gpiote_task_configure(0, CALLIOPE_PIN_MOTOR_IN1, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
    nrf_gpiote_task_enable(0);
    //task 1
    nrf_gpiote_task_configure(1, CALLIOPE_PIN_MOTOR_IN2, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
    nrf_gpiote_task_enable(1);

    //Three NOPs are required to make sure configuration is written before setting tasks or getting events
    __NOP();
    __NOP();
    __NOP();

    //connect the tasks to the corresponding compare match events, toggle twice per period (PWM)
    //connect task 0
    NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[0];
    NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
    //connect task 1
    NRF_PPI->CH[1].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[1];
    NRF_PPI->CH[1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];

    //connect task 0
    NRF_PPI->CH[2].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[2];
    NRF_PPI->CH[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
    //connect task 1
    NRF_PPI->CH[3].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[3];
    NRF_PPI->CH[3].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[1];

    NRF_PPI->CHENSET = 15; // bits 0 - 3 for channels 0 - 3

     //init TIMER2 for PWM use
    NRF_TIMER2->POWER = 1;
    NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
    NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
    NRF_TIMER2->PRESCALER = CALLIOPE_SM_PRESCALER_M;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //initialize compare registers
    //set compare registers 0 and 1 (duty cycle for PWM on pins CALLIOPE_PIN_MOTOR_IN1 and CALLIOPE_PIN_MOTOR_IN2)
    NRF_TIMER2->CC[0] = CALLIOPE_SM_PERIOD_M - CALLIOPE_SM_DEFAULT_DUTY_M;
    NRF_TIMER2->CC[1] = CALLIOPE_SM_DEFAULT_DUTY_M-1;
    //set compare register 2 and 3 (period for PWM on pins CALLIOPE_PIN_MOTOR_IN1 and CALLIOPE_PIN_MOTOR_IN2)
    NRF_TIMER2->CC[2] = CALLIOPE_SM_PERIOD_M-1;
    NRF_TIMER2->CC[3] = CALLIOPE_SM_PERIOD_M;
    NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MOTOR CONTROL FUNCTIONS - SINGLE MOTOR USE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//NOTE: the use of a motor control function will turn the sound off      
//functions to control the motor
void CalliopeSoundMotor::motorOn(int8_t duty_percent)
{
    //if value is out of bounds, do nothing
    if((duty_percent > 100) || (duty_percent < -100)) return;

    //set mode to single motor use
    mode = 1;

    //set current use of dual motor mode
    motor_AB_current_use = 0;

    //save current setting
    duty_motor_percent = duty_percent;

    //stop & clear timer -> non interrupt based change of duty cycle
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //set prescaler for motor use
    NRF_TIMER2->PRESCALER = CALLIOPE_SM_PRESCALER_M;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set compare register 2 and 3 for motor use (this sets the PWM period)
    NRF_TIMER2->CC[2] = CALLIOPE_SM_PERIOD_M-1;
    NRF_TIMER2->CC[3] = CALLIOPE_SM_PERIOD_M;

    //activate controller
    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_SLEEP);

    //set duty
    if(duty_percent == 0) {
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);
    }

    else if(duty_percent > 0) {
        if(duty_percent == 100){
            //set pins
            nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN1);
            nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);
        }
        else {
            //set pins for starting pwm again
            nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
            nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);

            //set duty cycle
            NRF_TIMER2->CC[0] = NRF_TIMER2->CC[3] - uint16_t((duty_percent * NRF_TIMER2->CC[3]) / 100);

            //enable task & restart PWM
            nrf_gpiote_task_enable(0);
            NRF_TIMER2->TASKS_START = 1;
        }
    }

    else if(duty_percent < 0) {
        if(duty_percent == -100){
            //set pins
            nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
            nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN2);
        }
        else {
            //set pins for starting PWM again
            nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
            nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN2);

            //set duty cycle
            NRF_TIMER2->CC[1] = uint16_t((duty_percent * -1 * NRF_TIMER2->CC[3]) / 100) - 1;

            //enable task & restart PWM
            nrf_gpiote_task_enable(1);
            NRF_TIMER2->TASKS_START = 1;
        }
    }
}


//function stops motor input -> motor will eventually stop, but is not actively braked
void CalliopeSoundMotor::motorCoast()
{
    //use function only for single motor use
    if (mode != 1) return;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set pins for motor coast use
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);
}


//function brakes motor
void CalliopeSoundMotor::motorBreak()
{
    //use function only for single motor use
    if (mode != 1) return;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set pins for motor break use
    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN1);
    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN2);
}


//function clears sleep pin -> motor behaves just like calling the coat function
void CalliopeSoundMotor::motorSleep()
{
    //use function only for single motor use
    if (mode != 1) return;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //clear pins
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);

    //deactivate controller & set mode to off
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_SLEEP);
    mode = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MOTOR CONTROL FUNCTIONS - DUAL MOTOR USE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CalliopeSoundMotor::motorAOn(uint8_t duty_percent)
{
    //if value is out of bounds, do nothing
    if(duty_percent > 100) return;

    //save current setting
    duty_motor_A_percent = duty_percent;

    //set mode to dual motor use
    mode = 2;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //set prescaler for motor use
    NRF_TIMER2->PRESCALER = CALLIOPE_SM_PRESCALER_M;

    //disable GPIOTE control of the PWM pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set compare register 2 and 3 for motor use (this sets the PWM period)
    NRF_TIMER2->CC[2] = CALLIOPE_SM_PERIOD_M-1;
    NRF_TIMER2->CC[3] = CALLIOPE_SM_PERIOD_M;

    //motors run at a maximum of 50% speed in dual motor use
    duty_percent = uint8_t(duty_percent/2);

    //set duty
    if(duty_percent != 0) {
        //set duty cycle for PWM controlling motor A
        NRF_TIMER2->CC[0] = CALLIOPE_SM_PERIOD_M - uint16_t((CALLIOPE_SM_PERIOD_M * duty_percent) / 100);
    }

//set current use -> A in use
    motor_AB_current_use |= 0x01;

    //values for duty cycle 0
    if((uint8_t(duty_motor_B_percent/2) == 0) || (motor_AB_current_use == 0x01)){
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);
    }
        //else set pins to default values
    else {
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
        nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN2);
    }

    //enable task for controlling motor A via PWM if duty > 0
    if(uint8_t(duty_motor_A_percent/2) != 0) nrf_gpiote_task_enable(0);

    //enable task for controlling motor B via PWM if motor B is in use
    if((uint8_t(duty_motor_B_percent/2) != 0) && (motor_AB_current_use == 3)) nrf_gpiote_task_enable(1);

    //restart timer
    NRF_TIMER2->TASKS_START = 1;

    //activate controller
    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_SLEEP);
}


void CalliopeSoundMotor::motorBOn(uint8_t duty_percent)
{
    //if value is out of bounds, do nothing
    if(duty_percent > 100) return;

    //save current setting
    duty_motor_B_percent = duty_percent;

    //set mode to dual motor use
    mode = 2;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //set prescaler for motor use
    NRF_TIMER2->PRESCALER = CALLIOPE_SM_PRESCALER_M;

    //disable GPIOTE control of the PWM pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set compare register 2 and 3 for motor use (this sets the PWM period)
    NRF_TIMER2->CC[2] = CALLIOPE_SM_PERIOD_M-1;
    NRF_TIMER2->CC[3] = CALLIOPE_SM_PERIOD_M;

    //motors run at a maximum of 50% speed in dual motor use
    duty_percent = uint8_t(duty_percent/2);

    //set duty
    if(duty_percent != 0) {
        //set duty cycle for PWM controlling motor B
        NRF_TIMER2->CC[1] = uint16_t((CALLIOPE_SM_PERIOD_M * duty_percent) / 100) - 1;
    }

    //set current use -> B in use
    motor_AB_current_use |= 0x02;

    //values for duty cycle 0
    if(uint8_t(duty_motor_B_percent/2) == 0 || (motor_AB_current_use == 0x02)) {
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);
    }
    //else set pins to default values
    else {
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
        nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN2);
    }

    //enable task for controlling motor A via PWM if motor A is in use
    if((uint8_t(duty_motor_A_percent/2) != 0)&& (motor_AB_current_use == 3)) nrf_gpiote_task_enable(0);

    //enable task for controlling motor B via PWM if duty > 0
    if((uint8_t(duty_motor_B_percent/2) != 0) ) nrf_gpiote_task_enable(1);

    //restart timer
    NRF_TIMER2->TASKS_START = 1;

    //activate controller
    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_SLEEP);
}


void CalliopeSoundMotor::motorAOff()
{
    //use function only for dual motor use
    if (mode != 2) return;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set current use of dual motor mode
    motor_AB_current_use &= ~(0x01);

    //PWM setup for motor A
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);

    if(motor_AB_current_use == 0) {
        //deactivate controller & set mode to off
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_SLEEP);
        mode = 0;
    }
    else {
        //set mode to dual motor use
        mode = 2;

        if(uint8_t(duty_motor_B_percent/2) != 0) {
            //start value for PWM on IN2
            nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN2);

            //enable task
            nrf_gpiote_task_enable(1);

            //restart timer
            NRF_TIMER2->TASKS_START = 1;

            //activate controller
            nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_SLEEP);
        }
    }
}


void CalliopeSoundMotor::motorBOff()
{
    //use function only for dual motor use
    if (mode != 2) return;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set current use of dual motor mode
    motor_AB_current_use &= ~(0x02);

    //PWM setup for motor B
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);

    if(motor_AB_current_use == 0) {
        //deactivate controller & set mode to off
        nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_SLEEP);
        mode = 0;
    }
    else {
        //set mode to dual motor use
        mode = 2;

        if(uint8_t(duty_motor_A_percent/2) != 0) {
            //start value for PWM on IN1
            nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);

            //enable task
            nrf_gpiote_task_enable(0);

            //restart timer
            NRF_TIMER2->TASKS_START = 1;

            //activate controller
            nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_SLEEP);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//SOUND CONTROL FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//functions to control the sound - NOTE: this will also effect the motor pins!
void CalliopeSoundMotor::soundOn(uint16_t frequency_hz)
{
    //if value is out of bounds, do nothing
    if((frequency_hz > CALLIOPE_MAX_FREQUENCY_HZ_S) || (frequency_hz < CALLIOPE_MIN_FREQUENCY_HZ_S)) return;

    //save current setting
    frequency_sound_hz = frequency_hz;

    //set mode to sound use
    mode = 3;

    //set current use of dual motor mode
    motor_AB_current_use = 0;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //set prescaler for sound use
    if(frequency_hz < CALLIOPE_MIN_FREQUENCY_HZ_S_NP) NRF_TIMER2->PRESCALER = CALLIOPE_SM_PRESCALER_S_LF;
    else NRF_TIMER2->PRESCALER = CALLIOPE_SM_PRESCALER_S;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set pins to default values 
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN2);

    //max 50% duty per pwm just like in dual motor use
    uint8_t duty = uint8_t(CALLIOPE_SM_DEFAULT_DUTY_S/2);

    //calculate period corresponding to the desired frequency and the currently used prescaler
    uint16_t period;
    if(frequency_hz < CALLIOPE_MIN_FREQUENCY_HZ_S_NP) period = uint16_t(uint32_t(CALLIOPE_BOARD_FREQUENCY) / (uint32_t(frequency_hz) << CALLIOPE_SM_PRESCALER_S_LF));
    else period = uint16_t(uint32_t(CALLIOPE_BOARD_FREQUENCY) / uint32_t(frequency_hz));

    //set compare register 2 and 3 according to the gives frequency (this sets the PWM period)
    NRF_TIMER2->CC[2] = period-1;
    NRF_TIMER2->CC[3] = period;

    //set duty cycle
    NRF_TIMER2->CC[0] = period - uint16_t(uint32_t((period * duty) / 100));
    NRF_TIMER2->CC[1] = uint16_t(uint32_t((period * duty) / 100)) - 1;

    //enable task & restart PWM depending on silent mode setting
    nrf_gpiote_task_enable(0);
    if(!silent_mode) nrf_gpiote_task_enable(1);
    NRF_TIMER2->TASKS_START = 1;

    //activate controller
    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_SLEEP);
}


void CalliopeSoundMotor::setSoundSilentMode(bool on_off)
{
    //set mode
    silent_mode = on_off;

    //return if sound is currently not in use
    if (mode != 3) return;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //set pins to default values 
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
    nrf_gpio_pin_set(CALLIOPE_PIN_MOTOR_IN2);

    //enable task & restart PWM depending on silent mode setting
    nrf_gpiote_task_enable(0);
    if(!silent_mode) nrf_gpiote_task_enable(1);
    NRF_TIMER2->TASKS_START = 1;
}



void CalliopeSoundMotor::soundOff()
{
    //use function only for sound use
    if (mode != 3) return;

    //stop & clear timer 
    NRF_TIMER2->TASKS_STOP = 1;
    NRF_TIMER2->TASKS_CLEAR = 1;

    //disable GPIOTE control of the pins
    nrf_gpiote_task_disable(0);
    nrf_gpiote_task_disable(1);

    //clear pins
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN1);
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_IN2);

    //deactivate controller & set mode to off
    nrf_gpio_pin_clear(CALLIOPE_PIN_MOTOR_SLEEP);
    mode = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CHECK AND GETTER FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//check fucntions
bool CalliopeSoundMotor::motorIsOn()
{
    if((mode == 1) || (mode == 2)) return true;
    else return false;
}

bool CalliopeSoundMotor::soundIsOn()
{
    if(mode == 3) return true;
    else return false;
}


//getter functions
uint8_t CalliopeSoundMotor::getMode()
{
    return mode;
}

int8_t CalliopeSoundMotor::motorGetDuty()
{
    return duty_motor_percent;
}

uint16_t CalliopeSoundMotor::soundGetFrequency()
{
    return frequency_sound_hz;
}


//periodic callback from MicroBit system timer.
void CalliopeSoundMotor::systemTick()
{
    //currently not in use
}
#endif