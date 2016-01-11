#ifndef DEBOUNCE_HAL_H
#define DEBOUNCE_HAL_H

#include "mbed.h"


#define MICROBIT_SIGMA_MAX               12
#define MICROBIT_SIGMA_THRESH_HI         10
#define MICROBIT_SIGMA_THRESH_LO         4

enum PinTransition
{
    LOW_LOW = 0,
    LOW_HIGH = 1,
    HIGH_LOW = 2,
    HIGH_HIGH = 3
};

class DebouncedPin
{

    DigitalIn pin;
    uint8_t low_threshold;
    uint8_t high_threshold;
    uint8_t maximum;
    uint8_t sigma;
    bool high;

    public:

        DebouncedPin(PinName name, bool start_high=true, uint8_t low_threshold=MICROBIT_SIGMA_THRESH_LO,
                     uint8_t high_threshold=MICROBIT_SIGMA_THRESH_HI, uint8_t maximum=MICROBIT_SIGMA_MAX,
                     PinMode mode = PullNone);

        PinTransition tick();

        bool isHigh() {
            return this->high;
        }

};

#endif