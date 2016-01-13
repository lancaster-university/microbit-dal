#include "mbed.h"

#ifndef MICROBIT_DYNAMIC_PWM_H
#define MICROBIT_DYNAMIC_PWM_H

#define NO_PWMS 3
#define MICROBIT_DEFAULT_PWM_PERIOD 20000

enum PwmPersistence
{
    PWM_PERSISTENCE_TRANSIENT = 1,
    PWM_PERSISTENCE_PERSISTENT = 2,
};

/**
  * Class definition for DynamicPwm.
  *
  * This class addresses a few issues found in the underlying libraries.
  * This provides the ability for a neat, clean swap between PWM channels.
  */
class DynamicPwm : public PwmOut
{
    private:
    static DynamicPwm* pwms[NO_PWMS];
    static uint8_t lastUsed;
    static uint16_t sharedPeriod;
    uint8_t flags;
    float lastValue;



    /**
      * An internal constructor used when allocating a new DynamicPwm representation
      * @param pin the name of the pin for the pwm to target
      * @param persistance the level of persistence for this pin PWM_PERSISTENCE_PERSISTENT (can not be replaced until freed, should only be used for system services really.)
      * or PWM_PERSISTENCE_TRANSIENT (can be replaced at any point if a channel is required.)
      */
    DynamicPwm(PinName pin, PwmPersistence persistence = PWM_PERSISTENCE_TRANSIENT);

    public:

    /**
      * Redirects the pwm channel to point at a different pin.
      * @param pin the new pin to direct PWM at.
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
      * pwm->redirect(PinName n2); // pwm is now produced on n2
      * @endcode
      */
    void redirect(PinName pin);


    /**
      * Retrieves a pointer to the first available free pwm channel - or the first one that can be reallocated.
      * @param pin the name of the pin for the pwm to target
      * @param persistance the level of persistence for this pin PWM_PERSISTENCE_PERSISTENT (can not be replaced until freed, should only be used for system services really.)
      * or PWM_PERSISTENCE_TRANSIENT (can be replaced at any point if a channel is required.)
      * @param period the frequency of the pwm channel in us.
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
      * @endcode
      */
    static DynamicPwm* allocate(PinName pin, PwmPersistence persistence = PWM_PERSISTENCE_TRANSIENT);

    /**
      * Frees this DynamicPwm instance if the pointer is valid.
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate();
      * pwm->release();
      * @endcode
      */
    void release();

    /**
      * A lightweight wrapper around the super class' write in order to capture the value
      *
      * @param value the duty cycle percentage in floating point format.
      *
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate();
      * pwm->write(0.5);
      * @endcode
      */
    int write(float value);

    /**
      * Retreives the pin name associated with this DynamicPwm instance.
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
      * pwm->getPinName();
      * @endcode
      */
    PinName getPinName();

    /**
      * Retreives the last value that has been written to this pwm channel.
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
      * pwm->getPeriod();
      * @endcode
      */
    int getValue();

    /**
      * Retreives the current period in use by the entire PWM module in microseconds.
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
      * pwm->getPeriod();
      * @endcode
      */
    int getPeriodUs();

    /**
      * Retreives the current period in use by the entire PWM module in milliseconds.
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
      * pwm->getPeriod();
      * @endcode
      */
    int getPeriod();

    /**
      * Sets the period used by the WHOLE PWM module. Any changes to the period will AFFECT ALL CHANNELS.
      *
      * @param period the desired period in microseconds.
      *
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
      * pwm->setPeriodUs(1000); // period now is 1ms
      * @endcode
      */
    int setPeriodUs(int period);

    /**
      * Sets the period used by the WHOLE PWM module. Any changes to the period will AFFECT ALL CHANNELS.
      *
      * @param period the desired period in milliseconds.
      *
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if value is out of range
      *
      * Example:
      * @code
      * DynamicPwm* pwm = DynamicPwm::allocate(PinName n);
      * pwm->setPeriod(1); // period now is 1ms
      * @endcode
      */
      int setPeriod(int period);
};

#endif
