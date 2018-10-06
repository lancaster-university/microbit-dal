#include "NRF51Timer.h"
#include "ErrorNo.h"
#include "MicroBitConfig.h"

static inline uint32_t counter_value(uint8_t cc)
{
    NRF_TIMER0->TASKS_CAPTURE[cc] = 1;
    return NRF_TIMER0->CC[cc];
}

static void (*timer_pointer) (uint8_t);

extern "C" void TIMER0_IRQHandler()
{
    uint8_t state = 0;

    for(int i = 0; i < TIMER_CHANNEL_COUNT; i++)
    {
        if(NRF_TIMER0->EVENTS_COMPARE[i])
        {
            state |= 1 << i;
            NRF_TIMER0->EVENTS_COMPARE[i] = 0;
        }
    }

    timer_pointer(state);
}

int NRF51Timer::setIRQ(void (*tp) (uint8_t))
{
    timer_pointer = tp;
    return MICROBIT_OK;
}

NRF51Timer::NRF51Timer(NRF_TIMER_Type* t, IRQn_Type irqn, uint8_t channel_count) : LowLevelTimer(channel_count)
{
    this->timer = t;
    this->irqn = irqn;
}

int NRF51Timer::enable()
{
    NVIC_SetVector(irqn, (uint32_t) TIMER0_IRQHandler);
    NVIC_SetPriority(irqn, 1);
    NVIC_ClearPendingIRQ(irqn);
    NVIC_EnableIRQ(irqn);

    timer->TASKS_START = 1;

    return MICROBIT_OK;
}

int NRF51Timer::disable()
{
    NVIC_DisableIRQ(irqn);
    timer->TASKS_STOP = 1;
    return MICROBIT_OK;
}

int NRF51Timer::setMode(TimerMode t)
{
    timer->MODE = t;
    return MICROBIT_OK;
}

int NRF51Timer::setCompare(uint8_t channel, uint32_t value)
{
    if (channel > getChannelCount() - 1)
        return MICROBIT_INVALID_PARAMETER;

    timer->CC[channel] = value;
    timer->INTENSET |= (1 << channel) << TIMER_INTENSET_COMPARE0_Pos;

    return MICROBIT_OK;
}

int NRF51Timer::offsetCompare(uint8_t channel, uint32_t value)
{
    if (channel > getChannelCount() - 1)
        return MICROBIT_INVALID_PARAMETER;

    timer->CC[channel] += value;
    timer->INTENSET |= (1 << channel) << TIMER_INTENSET_COMPARE0_Pos;

    return MICROBIT_OK;
}

int NRF51Timer::clearCompare(uint8_t channel)
{
    if (channel > getChannelCount() - 1)
        return MICROBIT_INVALID_PARAMETER;

    timer->INTENCLR |= (1 << channel) << TIMER_INTENCLR_COMPARE0_Pos;
    return MICROBIT_OK;
}

uint32_t NRF51Timer::captureCounter(uint8_t channel)
{
    if (channel > getChannelCount() - 1)
        return 0;

    return counter_value(channel);
}

int NRF51Timer::setPrescaler(uint16_t prescaleValue)
{
    if(prescaleValue > 0x09)
        return MICROBIT_INVALID_PARAMETER;

    timer->PRESCALER = prescaleValue;

    return MICROBIT_OK;
}

int NRF51Timer::setBitMode(TimerBitMode t)
{
    timer->BITMODE = t << TIMER_BITMODE_BITMODE_Pos;
    return MICROBIT_OK;
}