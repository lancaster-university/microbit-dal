/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

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
      * \par
      * For reference see PAN56 in:
      * \par
      * https://www.nordicsemi.com/eng/nordic/Products/nRF51822/PAN-nRF51822/24634
      * \par
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

    /**
     * Issues a standard, 2 byte I2C command write.
     *
     * Blocks the calling thread until complete.
     *
     * @param address The address of the I2C device to write to.
     * @param reg The address of the register in the device to write.
     * @param value The value to write.
     *
     * @return MICROBIT_OK on success, MICROBIT_I2C_ERROR if the the write request failed.
     */
    int writeRegister(uint8_t address, uint8_t reg, uint8_t value);

    /**
     * Issues a read command, copying data into the specified buffer.
     *
     * Blocks the calling thread until complete.
     *
     * @param reg The address of the register to access.
     *
     * @param buffer Memory area to read the data into.
     *
     * @param length The number of bytes to read.
     *
     * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER or MICROBIT_I2C_ERROR if the the read request failed.
     */
    int readRegister(uint8_t address, uint8_t reg, uint8_t* buffer, int length);

    /**
     * Issues a single byte read command, and returns the value read, or an error.
     *
     * Blocks the calling thread until complete.
     *
     * @param address The address of the I2C device to write to.
     * @param reg The address of the register to access.
     *
     * @return the byte read on success, MICROBIT_INVALID_PARAMETER or MICROBIT_I2C_ERROR if the the read request failed.
     */
    int readRegister(uint8_t address, uint8_t reg);

};

#endif
