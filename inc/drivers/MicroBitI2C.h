#ifndef MICROBIT_I2C_H
#define MICROBIT_I2C_H

#include "mbed.h"
#include "MicroBitConfig.h"

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
      *
      * Create an instance of MicroBitI2C for I2C communication.
      *
      * @param sda the Pin to be used for SDA
      *
      * @param scl the Pin to be used for SCL
      *
      * @code
      * MicroBitI2C i2c(I2C_SDA0, I2C_SCL0);
      * @endcode
      *
      * @note This class presents a wrapped mbed call to capture failed I2C operations caused by a known silicon bug in the nrf51822.
      * Attempts to automatically reset and restart the I2C hardware if this case is detected.
      *
      * For reference see PAN56 in:
      *
      * https://www.nordicsemi.com/eng/nordic/Products/nRF51822/PAN-nRF51822/24634
      *
      * v2.0 through to v2.4
      */
    MicroBitI2C(PinName sda, PinName scl);

    /**
      * Performs a complete read transaction. The bottom bit of the address is forced to 1 to indicate a read.
      *
      * @param address 8-bit I2C slave address [ addr | 1 ]
      *
      * @param data A pointer to a byte buffer used for storing retrieved data.
      *
      * @param length Number of bytes to read.
      *
      * @param repeated if true, stop is not sent at the end. Defaults to false.
      *
      * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if an unresolved read failure is detected.
      */
    int read(int address, char *data, int length, bool repeated = false);

    /**
      * Performs a complete write transaction. The bottom bit of the address is forced to 0 to indicate a write.
      *
      * @param address 8-bit I2C slave address [ addr | 0 ]
      *
      * @param data A pointer to a byte buffer containing the data to write.
      *
      * @param length Number of bytes to write
      *
      * @param repeated if true, stop is not sent at the end. Defaults to false.
      *
      * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if an unresolved write failure is detected.
      */
    int write(int address, const char *data, int length, bool repeated = false);
};

#endif
