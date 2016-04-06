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
  
#ifndef MICROBIT_COMPAT_H
#define MICROBIT_COMPAT_H

#include "ErrorNo.h"

#define PI 3.14159265359

/**
  * Determines the smallest of the two numbers
  * @param a the first number
  * @param b the second number
  * @return The value of a or b that is the smallest
  */
inline int min(int a, int b)
{
    return (a < b ? a : b);
}

/**
  * Determines the largest of the two numbers
  * @param a the first number
  * @param b the second number
  * @return The value of a or b that is the largest
  */
inline int max(int a, int b)
{
    return (a > b ? a : b);
}

/**
  * Sets a given area of memory to zero.
  *
  * @param a the pointer to the beginning of the memory to clear
  * @param b the number of bytes to clear.
  */
inline void *memclr(void *a, size_t b)
{
    return memset(a,0,b);
}

/**
  * Determines if the given character is a printable ASCII/UTF8 decimal digit (0..9).
  * @param c the character to check
  * @return true if the character is a digit, false otherwise.
  */
inline bool isdigit(char c)
{
    return (c > 47 && c < 58);
}

/**
  * Performs an in buffer reverse of a given char array
  * @param s the string to reverse.
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  */
int string_reverse(char *s);

/**
  * Converts a given integer into a string representation.
  * @param n The number to convert.
  * @param s A pointer to the buffer in which to store the resulting string.
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  */
void itoa(int n, char *s);


#endif
