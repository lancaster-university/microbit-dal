#ifndef MICROBIT_I2C_H
#define MICROBIT_I2C_H

#include "mbed.h"

#define MICROBIT_I2C_MAX_RETRIES 9

/**
  * Class definition for MicroBitI2C.
  *
  * Presents a wrapped mbed call to capture failed I2C operations caused by a known silicon bug in the nrf51822.
  * Attempts to automatically reset and restart the I2C hardware if this case is detected.
  * 
  * For reference see PAN56 in: 
  * 
  * https://www.nordicsemi.com/eng/nordic/Products/nRF51822/PAN-nRF51822/24634
  * 
  * v2.0 through to v2.4
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
  
    /**
      * Performs a complete read transaction. The bottom bit of the address is forced to 1 to indicate a read.
      *
      * @address 8-bit I2C slave address [ addr | 1 ]
      * @data Pointer to the byte-array to read data in to
      * @length Number of bytes to read
      * @repeated Repeated start, true - don't send stop at end.
      *
      * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if an unresolved read failure is detected.
      */
    int read(int address, char *data, int length, bool repeated = false);
   
    /**
      * Performs a complete write transaction. The bottom bit of the address is forced to 0 to indicate a write.
      *
      * @address 8-bit I2C slave address [ addr | 0 ]
      * @data Pointer to the byte-arraycontaining the data to write 
      * @length Number of bytes to write
      * @repeated Repeated start, true - don't send stop at end.
      *
      * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if an unresolved write failure is detected.
      */
    int write(int address, const char *data, int length, bool repeated = false);
};

#endif
