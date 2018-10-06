#ifndef MICROBIT_PERIDO_RADIO_TIMER_H
#define MICROBIT_PERIDO_RADIO_TIMER_H

#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "LowLevelTimer.h"

struct ClockState
{
    uint8_t state;
    uint32_t ticks;
    void (*timer_handler)();
};

class HigherLevelTimer : MicroBitComponent
{
    int triggerAfter(uint8_t channel, uint32_t ticks, void (*timer_handler)(), bool repeating, bool relative);
    uint8_t allocateChannel();

    public:
    static HigherLevelTimer* instance;
    LowLevelTimer& timer;
    ClockState* clock_state;

    HigherLevelTimer(LowLevelTimer& t);
    int triggerAt(uint32_t ticks, void (*timer_handler)());
    int triggerIn(uint32_t ticks, void (*timer_handler)());
    int triggerEvery(uint32_t ticks, void (*timer_handler)());

    ~HigherLevelTimer();
};

#endif