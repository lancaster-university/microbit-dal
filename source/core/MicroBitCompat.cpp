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
  * This file contains functions used to maintain compatability and portability.
  * It also contains constants that are used elsewhere in the DAL.
  */
#include "MicroBitConfig.h"
#include "MicroBitCompat.h"
#include "ErrorNo.h"


/**
  * Performs an in buffer reverse of a given char array.
  *
  * @param s the string to reverse.
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  */
int string_reverse(char *s)
{
    //sanity check...
    if(s == NULL)
        return MICROBIT_INVALID_PARAMETER;

    char *j;
    int c;

    j = s + strlen(s) - 1;

    while(s < j)
    {
        c = *s;
        *s++ = *j;
        *j-- = c;
    }

    return MICROBIT_OK;
}

/**
  * Converts a given integer into a string representation.
  *
  * @param n The number to convert.
  *
  * @param s A pointer to the buffer where the resulting string will be stored.
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  */
int itoa(int n, char *s)
{
    int i = 0;
    int positive = (n >= 0);

    if (s == NULL)
        return MICROBIT_INVALID_PARAMETER;

    // Record the sign of the number,
    // Ensure our working value is positive.
    if (positive)
        n = -n;

    // Calculate each character, starting with the LSB.
    do {
         s[i++] = abs(n % 10) + '0';
    } while (abs(n /= 10) > 0);

    // Add a negative sign as needed
    if (!positive)
        s[i++] = '-';

    // Terminate the string.
    s[i] = '\0';

    // Flip the order.
    string_reverse(s);

    return MICROBIT_OK;
}
