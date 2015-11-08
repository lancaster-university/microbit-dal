#ifndef MICROBIT_HAL_H
#define MICROBIT_HAL_H

#include "hal/MicroBitI2C.h"

// mbed pin assignments of core components.
#define MICROBIT_PIN_SDA                P0_30
#define MICROBIT_PIN_SCL                P0_0

/**
  * Class definition for a minimal abstract layer of the MicroBit device.
  *
  * Represents the device as a whole, and includes member variables to that reflect the components of the system.
  */
class MicroBitHal
{

    public:

    // I2C Interface
    static MicroBitI2C i2c;

};

#endif