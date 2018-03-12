#ifndef NRF51_TIMER_H
#define NRF51_TIMER_H

#include "LowLevelTimer.h"
#include "nrf51.h"

#define TIMER_CHANNEL_COUNT              4

class NRF51Timer : public LowLevelTimer
{
    IRQn_Type irqn;

    public:

    NRF_TIMER_Type *timer;

    int setIRQ(void (*timer_pointer) (uint8_t));

    NRF51Timer(NRF_TIMER_Type *timer, IRQn_Type irqn, uint8_t channel_count);

    int enable();

    int disable();

    int setMode(TimerMode t);

    int setCompare(uint8_t channel, uint32_t value);

    int offsetCompare(uint8_t channel, uint32_t value);

    int clearCompare(uint8_t channel);

    uint32_t captureCounter(uint8_t channel);

    int setPrescaler(uint16_t prescaleValue);

    int setBitMode(TimerBitMode t);
};

#endif