#include "HigherLevelTimer.h"
#include "ErrorNo.h"
#include "MicroBitConfig.h"
#include <stdint.h>

#define HIGHER_TIMER_STATE_UNUSED               0x01
#define HIGHER_TIMER_STATE_REPEATING            0x02
#define HIGHER_TIMER_STATE_IRQ                  0x04

HigherLevelTimer* HigherLevelTimer::instance = NULL;

void channel_handler(uint8_t state)
{
    if(HigherLevelTimer::instance == NULL)
        return;

    for(int i = 0; i < 4; i++)
    {
        if(!(state & (1<<i)))
            continue;

        if(HigherLevelTimer::instance->clock_state[i].state & HIGHER_TIMER_STATE_REPEATING)
            HigherLevelTimer::instance->timer.offsetCompare(i, HigherLevelTimer::instance->clock_state[i].ticks);
        else
        {
            HigherLevelTimer::instance->clock_state[i].state |= HIGHER_TIMER_STATE_UNUSED;
            HigherLevelTimer::instance->timer.clearCompare(i);
        }


        HigherLevelTimer::instance->clock_state[i].state |= HIGHER_TIMER_STATE_IRQ;
    }

    for(int i = 0; i < 4; i++)
    {
        if(HigherLevelTimer::instance->clock_state[i].state & HIGHER_TIMER_STATE_IRQ)
        {
            HigherLevelTimer::instance->clock_state[i].state &= ~HIGHER_TIMER_STATE_IRQ;
            HigherLevelTimer::instance->clock_state[i].timer_handler();
        }
    }
}

HigherLevelTimer::HigherLevelTimer(LowLevelTimer& t) : timer(t)
{
    instance = this;

    this->clock_state = (ClockState*)malloc(sizeof(ClockState) * t.getChannelCount());

    for(int i = 0; i < t.getChannelCount(); i++)
        clock_state[i].state = HIGHER_TIMER_STATE_UNUSED;

    timer.setIRQ(channel_handler);
}

uint8_t HigherLevelTimer::allocateChannel()
{
    uint8_t channel = 255;

    for(int i = 0; i < timer.getChannelCount(); i++)
        if(clock_state[i].state & HIGHER_TIMER_STATE_UNUSED)
        {
            channel = i;
            break;
        }

    return channel;
}

int HigherLevelTimer::triggerAt(uint32_t ticks, void (*timer_handler)())
{
    return triggerAfter(allocateChannel(), ticks, timer_handler, false, false);
}


int HigherLevelTimer::triggerAfter(uint8_t channel, uint32_t ticks, void (*timer_handler)(), bool repeating, bool relative)
{
    if (channel == 255)
        return MICROBIT_NO_RESOURCES;

    clock_state[channel].state &= ~HIGHER_TIMER_STATE_UNUSED;

    if (repeating)
        clock_state[channel].state |= HIGHER_TIMER_STATE_REPEATING;

    clock_state[channel].timer_handler = timer_handler;
    clock_state[channel].ticks = ticks;

    timer.setCompare(channel, ((relative) ? timer.captureCounter(channel) + ticks : ticks));

    return channel;
}

int HigherLevelTimer::triggerIn(uint32_t ticks, void (*timer_handler)())
{
    return triggerAfter(allocateChannel(), ticks , timer_handler, false, true);
}


int HigherLevelTimer::triggerEvery(uint32_t ticks, void (*timer_handler)())
{
    return triggerAfter(allocateChannel(), ticks, timer_handler, true, true);
}

HigherLevelTimer::~HigherLevelTimer()
{
    instance = NULL;
}