#include "hal/DebouncedPin.h"

/**
  * Constructor.
  * Create a debounced pin representation.
  * @param name the physical pin on the processor that this butotn is connected to.
  * @param start_high Whether the debounced start should start high.
  * @param low_threshold The threshold for determining a high to low transition.
  * @param high_threshold The threshold for determining a low to high transition.
  * @param maximum The high limit for debouncer state.
  * @param mode the configuration of internal pullups/pulldowns, as define in the mbed PinMode class. PullNone by default.
  */
DebouncedPin::DebouncedPin(PinName name, bool start_high, uint8_t low_threshold, uint8_t high_threshold, uint8_t maximum, PinMode mode) : pin(name, mode)
{
    this->low_threshold = low_threshold;
    this->high_threshold = high_threshold;
    this->maximum = maximum;

    this->high = start_high;
    if (start_high) {
        this->sigma = maximum;
    } else {
        this->sigma = 0;
    }
}
/**
 * Call back function for system ticker.
 * Measures whether the underlying pin is high/low and returns the debounced transition.
 */
PinTransition DebouncedPin::tick()
{
    if(pin)
    {
        if (sigma < maximum) {
            sigma++;
        }
    }
    else
    {
        if (sigma > 0) {
            sigma--;
        }
    }
    if (high) {
        // Is there a transition to low?
        if(sigma < low_threshold) {
            high = false;
            return HIGH_LOW;
        }
        return HIGH_HIGH;
    } else {
        // Is there a transition to high?
        if(sigma > high_threshold) {
            high = true;
            return LOW_HIGH;
        }
        return LOW_LOW;
    }
}
