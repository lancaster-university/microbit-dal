#ifndef LOW_LEVEL_TIMER_H
#define LOW_LEVEL_TIMER_H

#include "MicroBitConfig.h"
#include "MicroBitComponent.h"

enum TimerMode
{
    TimerModeTimer = 0,
    TimerModeCounter
};

enum TimerBitMode
{
    BitMode16 = 0,
    BitMode8,
    BitMode24,
    BitMode32
};

class LowLevelTimer : public MicroBitComponent
{
    protected:
    uint8_t channel_count;

    public:

    virtual int setIRQ(void (*timer_pointer) (uint8_t));

    LowLevelTimer(uint8_t channel_count)
    {
        this->channel_count = channel_count;
    }

    virtual int enable() = 0;

    virtual int disable() = 0;

    virtual int setMode(TimerMode t) = 0;

    virtual int setCompare(uint8_t channel, uint32_t value)= 0;

    virtual int offsetCompare(uint8_t channel, uint32_t value) = 0;

    virtual int clearCompare(uint8_t channel);

    virtual uint32_t captureCounter(uint8_t channel) = 0;

    // a better abstraction would be to set the timer tick granularity, us, ms, s etc.

    virtual int setPrescaler(uint16_t prescaleValue) = 0;

    virtual int setBitMode(TimerBitMode t) = 0;

    int getChannelCount()
    {
        return channel_count;
    }
};

#endif