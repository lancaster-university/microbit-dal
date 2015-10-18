#ifndef MICROBIT_PANIC_H
#define MICROBIT_PANIC_H

/**
  * Displays "=(" and an accompanying status code.
  * @param statusCode the appropriate status code - 0 means no code will be displayed. Status codes must be in the range 0-255.
  */
void panic(int statusCode);

/**
 * Resets the micro:bit.
 * @param statusCode the appropriate status code. Status codes must be in the range 0-255.
  */
void microbit_reset();

#endif

