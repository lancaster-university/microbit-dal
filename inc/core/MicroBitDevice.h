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

/**
 * Device specific funcitons for the nrf51822 device.
 *
 * Provides a degree of platform independence for microbit-dal functionality.
 *
 * TODO: Determine if any of this belongs in an mbed target definition.
 * TODO: Review microbit-dal to place all such functions here.
 */
#ifndef MICROBIT_DEVICE_H
#define MICROBIT_DEVICE_H

#define MICROBIT_NAME_LENGTH                    5
#define MICROBIT_NAME_CODE_LETTERS              5
#define MICROBIT_PANIC_ERROR_CHARS              4

#include "MicroBitConfig.h"
#include "MicroBitMatrixMaps.h"

/**
  * Determines if a BLE stack is currently running.
  *
  * @return true is a bluetooth stack is operational, false otherwise.
  */
bool ble_running();

/**
 * Derive a unique, consistent serial number of this device from internal data.
 *
 * @return the serial number of this device.
 */
uint32_t microbit_serial_number();

/**
 * Derive the friendly name for this device, based on its serial number.
 *
 * @return the serial number of this device.
 */
char* microbit_friendly_name();

/**
  * Perform a hard reset of the micro:bit.
  */
void microbit_reset();

/**
  * Determine the version of microbit-dal currently running.
  * @return a pointer to a character buffer containing a representation of the semantic version number.
  */
const char * microbit_dal_version();

/**
  * Disables all interrupts and user processing.
  * Displays "=(" and an accompanying status code on the default display.
  * @param statusCode the appropriate status code, must be in the range 0-999.
  *
  * @code
  * microbit_panic(20);
  * @endcode
  */
void microbit_panic(int statusCode);

/**
 * Defines the length of time that the device will remain in a error state before resetting.
 *
 * @param iteration The number of times the error code will be displayed before resetting. Set to zero to remain in error state forever.
 *
 * @code
 * microbit_panic_timeout(4);
 * @endcode
 */
void microbit_panic_timeout(int iterations);

/**
  * Generate a random number in the given range.
  * We use a simple Galois LFSR random number generator here,
  * as a Galois LFSR is sufficient for our applications, and much more lightweight
  * than the hardware random number generator built int the processor, which takes
  * a long time and uses a lot of energy.
  *
  * KIDS: You shouldn't use this is the real world to generte cryptographic keys though...
  * have a think why not. :-)
  *
  * @param max the upper range to generate a number for. This number cannot be negative.
  *
  * @return A random, natural number between 0 and the max-1. Or MICROBIT_INVALID_VALUE if max is <= 0.
  *
  * @code
  * microbit_random(200); //a number between 0 and 199
  * @endcode
  */
int microbit_random(int max);

/**
  * Seed the random number generator (RNG).
  *
  * This function uses the NRF51822's in built cryptographic random number generator to seed a Galois LFSR.
  * We do this as the hardware RNG is relatively high power, and is locked out by the BLE stack internally,
  * with a less than optimal application interface. A Galois LFSR is sufficient for our
  * applications, and much more lightweight.
  */
void microbit_seed_random();

/**
  * Seed the pseudo random number generator (RNG) using the given 32-bit value.
  * This function does not use the NRF51822's in built cryptographic random number generator.
  *
  * @param seed The value to use as a seed.
  */
void microbit_seed_random(uint32_t seed);

#endif
