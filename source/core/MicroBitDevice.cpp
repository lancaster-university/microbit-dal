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
  * Compatibility / portability funcitons and constants for the MicroBit DAL.
  */
#include "MicroBitConfig.h"
#include "MicroBitButton.h"
#include "MicroBitDevice.h"
#include "MicroBitFont.h"
#include "mbed.h"
#include "ErrorNo.h"

/*
 * The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
#if !defined(__arm)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "nrf_soc.h"
#include "nrf_sdm.h"

/*
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif

static char friendly_name[MICROBIT_NAME_LENGTH+1];
static const uint8_t panicFace[5] = {0x1B, 0x1B,0x0,0x0E,0x11};
static int panic_timeout = 0;
static uint32_t random_value = 0;

/**
  * Determines if a BLE stack is currently running.
  *
  * @return true is a bluetooth stack is operational, false otherwise.
  */
bool ble_running()
{
    uint8_t t = 0;

#if CONFIG_ENABLED(MICROBIT_BLE_ENABLED) || CONFIG_ENABLED(MICROBIT_BLE_PAIRING_MODE)
    sd_softdevice_is_enabled(&t);
#endif

    return t==1;
}

/**
 * Derive a unique, consistent serial number of this device from internal data.
 *
 * @return the serial number of this device.
 */
uint32_t microbit_serial_number()
{
    return NRF_FICR->DEVICEID[1];
}

/**
 * Derive the friendly name for this device, based on its serial number.
 *
 * @return the serial number of this device.
 */
char* microbit_friendly_name()
{
    const uint8_t codebook[MICROBIT_NAME_LENGTH][MICROBIT_NAME_CODE_LETTERS] =
    {
        {'z', 'v', 'g', 'p', 't'},
        {'u', 'o', 'i', 'e', 'a'},
        {'z', 'v', 'g', 'p', 't'},
        {'u', 'o', 'i', 'e', 'a'},
        {'z', 'v', 'g', 'p', 't'}
    };

    // We count right to left, so create a pointer to the end of the buffer.
	char *name = friendly_name;
    name += MICROBIT_NAME_LENGTH;

    // Terminate the string.
    *name = 0;

	// Derive our name from the nrf51822's unique ID.
    uint32_t n = microbit_serial_number();
    int ld = 1;
    int d = MICROBIT_NAME_CODE_LETTERS;
    int h;

    for (int i=0; i<MICROBIT_NAME_LENGTH; i++)
    {
        h = (n % d) / ld;
        n -= h;
        d *= MICROBIT_NAME_CODE_LETTERS;
        ld *= MICROBIT_NAME_CODE_LETTERS;
        *--name = codebook[i][h];
    }

    return friendly_name;
}

/**
  * Perform a hard reset of the micro:bit.
  */
void
microbit_reset()
{
    NVIC_SystemReset();
}

/**
  * Determine the version of microbit-dal currently running.
  * @return a pointer to a character buffer containing a representation of the semantic version number.
  */
const char *
microbit_dal_version()
{
    return MICROBIT_DAL_VERSION;
}

/**
 * Defines the length of time that the device will remain in a error state before resetting.
 *
 * @param iteration The number of times the error code will be displayed before resetting. Set to zero to remain in error state forever.
 *
 * @code
 * microbit_panic_timeout(4);
 * @endcode
 */
void microbit_panic_timeout(int iterations)
{
    panic_timeout = iterations;
}

/**
  * Disables all interrupts and user processing.
  * Displays "=(" and an accompanying status code on the default display.
  * @param statusCode the appropriate status code, must be in the range 0-999.
  *
  * @code
  * microbit_panic(20);
  * @endcode
  */
void microbit_panic(int statusCode)
{
    DigitalIn resetButton(MICROBIT_PIN_BUTTON_RESET);
    resetButton.mode(PullUp);

    uint32_t    row_mask = 0;
    uint32_t    col_mask = 0;
    uint32_t    row_reset = 0x01 << microbitMatrixMap.rowStart;
    uint32_t    row_data = row_reset;
    uint8_t     count = panic_timeout ? panic_timeout : 1;
    uint8_t     strobeRow = 0;

    row_mask = 0;
    for (int i = microbitMatrixMap.rowStart; i < microbitMatrixMap.rowStart + microbitMatrixMap.rows; i++)
        row_mask |= 0x01 << i;

    for (int i = microbitMatrixMap.columnStart; i < microbitMatrixMap.columnStart + microbitMatrixMap.columns; i++)
        col_mask |= 0x01 << i;

    PortOut LEDMatrix(Port0, row_mask | col_mask);

    if(statusCode < 0 || statusCode > 999)
        statusCode = 0;

    __disable_irq(); //stop ALL interrupts


    //point to the font stored in Flash
    const unsigned char* fontLocation = MicroBitFont::defaultFont;

    //get individual digits of status code, and place it into a single array/
    const uint8_t* chars[MICROBIT_PANIC_ERROR_CHARS] = { panicFace, fontLocation+((((statusCode/100 % 10)+48)-MICROBIT_FONT_ASCII_START) * 5), fontLocation+((((statusCode/10 % 10)+48)-MICROBIT_FONT_ASCII_START) * 5), fontLocation+((((statusCode % 10)+48)-MICROBIT_FONT_ASCII_START) * 5)};

    while(count)
    {
        //iterate through our chars :)
        for(int characterCount = 0; characterCount < MICROBIT_PANIC_ERROR_CHARS; characterCount++)
        {
            int outerCount = 0;

            //display the current character
            while(outerCount < 500)
            {
                uint32_t col_data = 0;

                int i = 0;

                //if we have hit the row limit - reset both the bit mask and the row variable
                if(strobeRow == microbitMatrixMap.rows)
                {
                    strobeRow = 0;
                    row_data = row_reset;
                }

                // Calculate the bitpattern to write.
                for (i = 0; i < microbitMatrixMap.columns; i++)
                {
                    int index = (i * microbitMatrixMap.rows) + strobeRow;

                    int bitMsk = 0x10 >> microbitMatrixMap.map[index].x; //chars are right aligned but read left to right
                    int y = microbitMatrixMap.map[index].y;

                    if(chars[characterCount][y] & bitMsk)
                        col_data |= (1 << i);
                }

                col_data = ~col_data << microbitMatrixMap.columnStart & col_mask;

                if(chars[characterCount] == chars[(characterCount - 1) % MICROBIT_PANIC_ERROR_CHARS] && outerCount < 50)
                    LEDMatrix =  0;
                else
                    LEDMatrix = col_data | row_data;

                //burn cycles
                i = 2000;
                while(i>0)
                {
                    // Check if the reset button has been pressed. Interrupts are disabled, so the normal method can't be relied upon...
                    if (resetButton == 0)
                        microbit_reset();

                    i--;
                }

                //update the bit mask and row count
                row_data <<= 1;
                strobeRow++;
                outerCount++;
            }
        }

        if (panic_timeout)
            count--;
    }

    microbit_reset();
}

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
int microbit_random(int max)
{
    uint32_t m, result;

    if(max <= 0)
        return MICROBIT_INVALID_PARAMETER;

    // Our maximum return value is actually one less than passed
    max--;

    do {
        m = (uint32_t)max;
        result = 0;
		do {
            // Cycle the LFSR (Linear Feedback Shift Register).
            // We use an optimal sequence with a period of 2^32-1, as defined by Bruce Schneier here (a true legend in the field!),
            // For those interested, it's documented in his paper:
            // "Pseudo-Random Sequence Generator for 32-Bit CPUs: A fast, machine-independent generator for 32-bit Microprocessors"
            // https://www.schneier.com/paper-pseudorandom-sequence.html
			uint32_t rnd = random_value;

            rnd = ((((rnd >> 31)
                          ^ (rnd >> 6)
                          ^ (rnd >> 4)
                          ^ (rnd >> 2)
                          ^ (rnd >> 1)
                          ^ rnd)
                          & 0x0000001)
                          << 31 )
                          | (rnd >> 1);

			random_value = rnd;

            result = ((result << 1) | (rnd & 0x00000001));
        } while(m >>= 1);
    } while (result > (uint32_t)max);

    return result;
}

/**
  * Seed the random number generator (RNG).
  *
  * This function uses the NRF51822's in built cryptographic random number generator to seed a Galois LFSR.
  * We do this as the hardware RNG is relatively high power, and is locked out by the BLE stack internally,
  * with a less than optimal application interface. A Galois LFSR is sufficient for our
  * applications, and much more lightweight.
  */
void microbit_seed_random()
{
    random_value = 0;

    if(ble_running())
    {
        // If Bluetooth is enabled, we need to go through the Nordic software to safely do this.
        uint32_t result = sd_rand_application_vector_get((uint8_t*)&random_value, sizeof(random_value));

        // If we couldn't get the random bytes then at least make the seed non-zero.
        if (result != NRF_SUCCESS)
            random_value = 0xBBC5EED;
    }
    else
    {
        // Othwerwise we can access the hardware RNG directly.

        // Start the Random number generator. No need to leave it running... I hope. :-)
        NRF_RNG->TASKS_START = 1;

        for(int i = 0; i < 4; i++)
        {
            // Clear the VALRDY EVENT
            NRF_RNG->EVENTS_VALRDY = 0;

            // Wait for a number ot be generated.
            while(NRF_RNG->EVENTS_VALRDY == 0);

            random_value = (random_value << 8) | ((int) NRF_RNG->VALUE);
        }

        // Disable the generator to save power.
        NRF_RNG->TASKS_STOP = 1;
    }
}

/**
  * Seed the pseudo random number generator (RNG) using the given 32-bit value.
  * This function does not use the NRF51822's in built cryptographic random number generator.
  *
  * @param seed The value to use as a seed.
  */
void microbit_seed_random(uint32_t seed)
{
    random_value = seed;
}
