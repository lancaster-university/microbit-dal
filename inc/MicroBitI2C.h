#include "MicroBit.h"
#ifndef MICROBIT_I2C_H
#define MICROBIT_I2C_H

#include "MicroBitComponent.h"
#include "mbed.h"

#define MICROBIT_I2C_MAX_RETRIES 9

/**
  * Class definition for MicroBitI2C.
  *
  * Represents a wrapped mbed call to hopefully fix silicon issues once and for all.
  */
class MicroBitI2C : public I2C
{
    
    uint8_t retries;
    
    public:
    
    /**
      * Constructor. 
      * Create an instance of i2c
      * @param sda the Pin to be used for SDA
      * @param scl the Pin to be used for SCL
      * Example:
      * @code 
      * MicroBitI2C i2c(MICROBIT_PIN_SDA, MICROBIT_PIN_SCL);
      * @endcode
      * @note this should prevent i2c lockups as well.
      */
    MicroBitI2C(PinName sda, PinName scl);
    
    int read(int address, char *data, int length, bool repeated = false);
    
    int write(int address, const char *data, int length, bool repeated = false);
};

#endif
